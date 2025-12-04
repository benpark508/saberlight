#include <stdint.h>
#include "../inc/mpu6500.h"
#include "../inc/SysTick.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/hw_types.h"
#include "../inc/SPI.h"

#define MPU_CS (*((volatile uint32_t *)0x40007040))
#define MPU_CS_LOW 0x00
#define MPU_CS_HIGH 0x10
#define MPU_READ 0x80
#define MPU_WRITE 0x00

long StartCritical(void);
void EndCritical(long sr);

static void WaitForSSI0Idle(void)
{
    while ((SSI0_SR_R & 0x10) == 0x10)
    {
    };
}

#define FILTER_SIZE 5
#define GYRO_DEADBAND_MDPS 2000

static int32_t accel_history[3][FILTER_SIZE] = {0};
static int32_t accel_sum[3] = {0};

static int32_t gyro_history[3][FILTER_SIZE] = {0};
static int32_t gyro_sum[3] = {0};

static uint8_t filter_idx = 0;

#define SWING_START_DPS 30
#define SWING_START_SQ (SWING_START_DPS * SWING_START_DPS)

#define SWING_STOP_DPS 20
#define SWING_STOP_SQ (SWING_STOP_DPS * SWING_STOP_DPS)

#define SWING_COOLDOWN_CYCLES 3

static uint8_t is_swinging_state = 0;
static int32_t cooldown_timer = 0;

uint8_t MPU6500_DetectSwing(processed_imu *proc)
{
    // Convert to degrees per second to avoid overflow when squaring
    int32_t x_dps = proc->gyro_x_mdps / 1000;
    int32_t y_dps = proc->gyro_y_mdps / 1000;
    int32_t z_dps = proc->gyro_z_mdps / 1000;

    // Calculate magnitude squared to avoid expensive square root operation
    int32_t mag_sq = (x_dps * x_dps) + (y_dps * y_dps) + (z_dps * z_dps);

    if (is_swinging_state == 0)
    {
        // Check cooldown timer to prevent rapid re-triggering
        if (cooldown_timer > 0)
        {
            cooldown_timer--;
            return 0;
        }

        // Check if magnitude exceeds high threshold to start swing
        if (mag_sq > SWING_START_SQ)
        {
            is_swinging_state = 1;
            return 1;
        }
    }
    else
    {
        // Check if magnitude drops below low threshold to stop swing
        if (mag_sq < SWING_STOP_SQ)
        {
            is_swinging_state = 0;
            cooldown_timer = SWING_COOLDOWN_CYCLES;
            return 0;
        }

        // Maintain swing state if within hysteresis buffer
        return 1;
    }

    return 0;
}

uint8_t MPU6500_ReadReg(uint8_t reg)
{
    long sr = StartCritical();
    WaitForSSI0Idle();

    uint8_t result;
    select_IMU();
    xchg_spi(reg | MPU_READ);
    result = xchg_spi(0xFF);
    deselect_IMU();

    WaitForSSI0Idle();
    EndCritical(sr);
    return result;
}

void MPU6500_Init(void)
{
    long sr = StartCritical();

    WaitForSSI0Idle();
    FCLK_SLOW();

    // Reset device
    select_IMU();
    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x80);
    deselect_IMU();

    WaitForSSI0Idle();
    FCLK_LCD();
    EndCritical(sr);

    SysTick80_Wait10ms(10);

    sr = StartCritical();
    WaitForSSI0Idle();
    FCLK_SLOW();

    // Reset signal paths
    select_IMU();
    xchg_spi(SIGNAL_PATH_RESET | MPU_WRITE);
    xchg_spi(0x07);
    deselect_IMU();

    WaitForSSI0Idle();
    FCLK_LCD();
    EndCritical(sr);

    SysTick80_Wait10ms(10);

    sr = StartCritical();
    WaitForSSI0Idle();
    FCLK_SLOW();

    // Wake up device
    select_IMU();
    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x00);
    deselect_IMU();

    select_IMU();
    xchg_spi(USER_CTRL | MPU_WRITE);
    xchg_spi(0x10);
    deselect_IMU();

    // Configure Digital Low Pass Filter
    select_IMU();
    xchg_spi(CONFIG | MPU_WRITE);
    xchg_spi(0x01);
    deselect_IMU();

    // Set Accel range to 8g
    select_IMU();
    xchg_spi(ACCEL_CONFIG | MPU_WRITE);
    xchg_spi(0x10);
    deselect_IMU();

    // Set Gyro range to 500dps
    select_IMU();
    xchg_spi(GYRO_CONFIG | MPU_WRITE);
    xchg_spi(0x18);
    deselect_IMU();

    WaitForSSI0Idle();
    FCLK_LCD();
    EndCritical(sr);
}

void MPU6500_ReadBlock(uint8_t reg, uint8_t *data, uint8_t length)
{
    long sr = StartCritical();
    WaitForSSI0Idle();

    select_IMU();
    xchg_spi(reg | MPU_READ);

    for (int i = 0; i < length; i++)
    {
        data[i] = xchg_spi(0xFF);
    }

    deselect_IMU();
    WaitForSSI0Idle();
    EndCritical(sr);
}

void MPU6500_calibrate(processed_imu *proc)
{
    int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;
    int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
    raw_imu temp_raw;
    int i;
    int samples = 100;

    for (i = 0; i < samples; i++)
    {
        MPU6500_read_accel(&temp_raw);
        MPU6500_read_gyro(&temp_raw);

        gx_sum += temp_raw.gyro_x;
        gy_sum += temp_raw.gyro_y;
        gz_sum += temp_raw.gyro_z;

        ax_sum += temp_raw.accel_x;
        ay_sum += temp_raw.accel_y;
        az_sum += temp_raw.accel_z;

        SysTick_Wait10ms(1);
    }

    proc->gyro_off_x = gx_sum / samples;
    proc->gyro_off_y = gy_sum / samples;
    proc->gyro_off_z = gz_sum / samples;

    proc->accel_off_x = ax_sum / samples;
    proc->accel_off_y = ay_sum / samples;

    // Remove gravity component (4096 LSB) from Z-axis offset
    proc->accel_off_z = (az_sum / samples) - 4096;
}

#define ACCEL_SCALE_FACTOR 4096
#define GYRO_SCALE_FACTOR 16

void MPU6500_ProcessData(raw_imu *raw, processed_imu *proc)
{
    int32_t raw_accel_mg[3];
    int32_t raw_gyro_mdps[3];

    raw_accel_mg[0] = ((int32_t)(raw->accel_x - proc->accel_off_x) * 1000) / ACCEL_SCALE_FACTOR;
    raw_gyro_mdps[0] = ((int32_t)(raw->gyro_x - proc->gyro_off_x) * 1000) / GYRO_SCALE_FACTOR;

    raw_accel_mg[1] = ((int32_t)(raw->accel_y - proc->accel_off_y) * 1000) / ACCEL_SCALE_FACTOR;
    raw_gyro_mdps[1] = ((int32_t)(raw->gyro_y - proc->gyro_off_y) * 1000) / GYRO_SCALE_FACTOR;

    raw_accel_mg[2] = ((int32_t)(raw->accel_z - proc->accel_off_z) * 1000) / ACCEL_SCALE_FACTOR;
    raw_gyro_mdps[2] = ((int32_t)(raw->gyro_z - proc->gyro_off_z) * 1000) / GYRO_SCALE_FACTOR;

    for (int i = 0; i < 3; i++)
    {
        // Accel Smoothing (Gravity)
        accel_sum[i] -= accel_history[i][filter_idx];
        accel_history[i][filter_idx] = raw_accel_mg[i];
        accel_sum[i] += raw_accel_mg[i];

        // Deadband (Noise Removal)
        if (raw_gyro_mdps[i] < GYRO_DEADBAND_MDPS && raw_gyro_mdps[i] > -GYRO_DEADBAND_MDPS)
        {
            raw_gyro_mdps[i] = 0;
        }
    }

    proc->accel_x_mg = accel_sum[0] / FILTER_SIZE;
    proc->accel_y_mg = accel_sum[1] / FILTER_SIZE;
    proc->accel_z_mg = accel_sum[2] / FILTER_SIZE;

    // Direct Pass-through for Gyro (No smoothing lag)
    proc->gyro_x_mdps = raw_gyro_mdps[0];
    proc->gyro_y_mdps = raw_gyro_mdps[1];
    proc->gyro_z_mdps = raw_gyro_mdps[2];

    filter_idx++;
    if (filter_idx >= FILTER_SIZE)
    {
        filter_idx = 0;
    }
}

void MPU6500_getData(raw_imu *rData, processed_imu *pData)
{
    MPU6500_read_accel(rData);
    MPU6500_read_gyro(rData);
    MPU6500_ProcessData(rData, pData);
}

void MPU6500_read_accel(raw_imu *imu_raw_data)
{
    uint8_t imu_data[6];
    MPU6500_ReadBlock(ACCEL_XOUT_H, imu_data, 6);

    imu_raw_data->accel_x = (int16_t)(((uint16_t)imu_data[0] << 8) | imu_data[1]);
    imu_raw_data->accel_y = (int16_t)(((uint16_t)imu_data[2] << 8) | imu_data[3]);
    imu_raw_data->accel_z = (int16_t)(((uint16_t)imu_data[4] << 8) | imu_data[5]);
}

void MPU6500_read_gyro(raw_imu *imu_raw_data)
{
    uint8_t imu_data[6];
    MPU6500_ReadBlock(GYRO_XOUT_H, imu_data, 6);

    imu_raw_data->gyro_x = (int16_t)(((uint16_t)imu_data[0] << 8) | imu_data[1]);
    imu_raw_data->gyro_y = (int16_t)(((uint16_t)imu_data[2] << 8) | imu_data[3]);
    imu_raw_data->gyro_z = (int16_t)(((uint16_t)imu_data[4] << 8) | imu_data[5]);
}