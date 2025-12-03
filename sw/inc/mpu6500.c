// mpu6500.c
// Runs on TM4C123
// driver for mpu6500 9-axis IMU
// Ben Park
// October 28 2025

#include <stdint.h>
#include "../inc/mpu6500.h"
#include "../inc/SysTick.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/hw_types.h"
#include "../inc/SPI.h"

#define MPU_CS (*((volatile uint32_t *)0x40007040)) // PD4 bit-band
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

#define FILTER_SIZE 10

// Static variables retain their value between function calls
// [3] represents axes: 0=X, 1=Y, 2=Z
static int32_t accel_history[3][FILTER_SIZE] = {0};
static int32_t accel_sum[3] = {0};
static uint8_t filter_idx = 0;

#define SWING_THRESHOLD_DPS 300 // 300 degrees/sec is a decent swing
#define SWING_THRESHOLD_SQ (SWING_THRESHOLD_DPS * SWING_THRESHOLD_DPS)
#define SWING_COOLDOWN_CYCLES 5 // Delay to prevent "machine gun" sound triggers

int32_t cooldown_timer = 0;

// Returns 1 if a swing is detected, 0 otherwise
uint8_t MPU6500_DetectSwing(processed_imu *proc)
{

    // 1. Handle Cooldown
    // If we just triggered a swing, wait a bit before allowing another
    if (cooldown_timer > 0)
    {
        cooldown_timer--;
        return 0;
    }

    // 2. Scale down to simple DPS to prevent overflow when squaring
    int32_t x_dps = proc->gyro_x_mdps / 1000;
    int32_t y_dps = proc->gyro_y_mdps / 1000;
    int32_t z_dps = proc->gyro_z_mdps / 1000;

    // 3. Calculate Magnitude Squared
    // MagSq = X^2 + Y^2 + Z^2
    int32_t mag_sq = (x_dps * x_dps) + (y_dps * y_dps) + (z_dps * z_dps);

    // 4. Compare to Threshold
    if (mag_sq > SWING_THRESHOLD_SQ)
    {
        // Swing detected!
        cooldown_timer = SWING_COOLDOWN_CYCLES; // Reset cooldown
        return 1;
    }

    return 0;
}

uint8_t MPU6500_ReadReg(uint8_t reg)
{
    long sr = StartCritical();
    WaitForSSI0Idle();

    // Note: No FCLK switching needed here if Main Loop is already Fast

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

    select_IMU();
    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x80); // Reset
    deselect_IMU();

    WaitForSSI0Idle();

    FCLK_LCD();

    EndCritical(sr);

    SysTick80_Wait10ms(10); // wait 100 ms

    sr = StartCritical();

    WaitForSSI0Idle();

    FCLK_SLOW();

    select_IMU();
    xchg_spi(SIGNAL_PATH_RESET | MPU_WRITE);
    xchg_spi(0x07); // Reset
    deselect_IMU();

    WaitForSSI0Idle();

    FCLK_LCD();

    EndCritical(sr);

    SysTick80_Wait10ms(10); // wait 100 ms

    sr = StartCritical();

    WaitForSSI0Idle();

    FCLK_SLOW();

    select_IMU();
    xchg_spi(PWR_MGMT_1 | MPU_WRITE);
    xchg_spi(0x00); // Wake Up
    deselect_IMU();

    select_IMU();
    xchg_spi(USER_CTRL | MPU_WRITE);
    xchg_spi(0x10); // Wake Up
    deselect_IMU();

    select_IMU();
    xchg_spi(CONFIG | MPU_WRITE);
    xchg_spi(0x03); // DLPF
    deselect_IMU();

    select_IMU();
    xchg_spi(ACCEL_CONFIG | MPU_WRITE);
    xchg_spi(0x10); // 8g
    deselect_IMU();

    select_IMU();
    xchg_spi(GYRO_CONFIG | MPU_WRITE);
    xchg_spi(0x08); // 500dps
    deselect_IMU();

    WaitForSSI0Idle();

    FCLK_LCD();

    EndCritical(sr);
}

void MPU6500_ReadBlock(uint8_t reg, uint8_t *data, uint8_t length)
{
    long sr = StartCritical();
    WaitForSSI0Idle();

    select_IMU();             // CS Low
    xchg_spi(reg | MPU_READ); // Send Start Register Address

    for (int i = 0; i < length; i++)
    {
        data[i] = xchg_spi(0xFF); // Send Dummy to clock in data
    }

    deselect_IMU(); // CS High
    WaitForSSI0Idle();
    EndCritical(sr);
}

void MPU6500_calibrate(processed_imu *proc)
{
    int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;
    int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
    raw_imu temp_raw;
    int i;
    int samples = 100; // Take 100 samples (~1 second)

    // 1. Accumulate samples
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

        SysTick_Wait10ms(1); // Wait 10ms between reads to capture noise
    }

    // 2. Average and store Gyro Offsets (Target is 0)
    proc->gyro_off_x = gx_sum / samples;
    proc->gyro_off_y = gy_sum / samples;
    proc->gyro_off_z = gz_sum / samples;

    // 3. Average and store Accel Offsets
    // Target for X and Y is 0
    proc->accel_off_x = ax_sum / samples;
    proc->accel_off_y = ay_sum / samples;

    // Target for Z is 1G (4096). The offset is the difference.
    // e.g. If Z reads 4200, Offset = 4200 - 4096 = 104.
    // Later: 4200 - 104 = 4096.
    proc->accel_off_z = (az_sum / samples) - 4096;
}

#define ACCEL_SCALE_FACTOR 4096 // LSB per g
#define GYRO_SCALE_FACTOR 65    // LSB per dps (approx 65.5)
#define DT_MS 100               // 100ms loop time
#define ALPHA 980               // 0.980 * 1000 (Filter coefficient)

void MPU6500_ProcessData(raw_imu *raw, processed_imu *proc)
{
    // 1. Convert Raw to Milli-Units
    // Formulat: (Raw - Offset) * 1000 / Sensitivity
    int32_t raw_x_mg = ((int32_t)(raw->accel_x - proc->accel_off_x) * 1000) / ACCEL_SCALE_FACTOR;
    int32_t raw_y_mg = ((int32_t)(raw->accel_y - proc->accel_off_y) * 1000) / ACCEL_SCALE_FACTOR;
    int32_t raw_z_mg = ((int32_t)(raw->accel_z - proc->accel_off_z) * 1000) / ACCEL_SCALE_FACTOR;

    accel_sum[0] -= accel_history[0][filter_idx];  // Subtract oldest value
    accel_history[0][filter_idx] = raw_x_mg;       // Overwrite with new value
    accel_sum[0] += raw_x_mg;                      // Add new value
    proc->accel_x_mg = accel_sum[0] / FILTER_SIZE; // Average

    // --- Y AXIS ---
    accel_sum[1] -= accel_history[1][filter_idx];
    accel_history[1][filter_idx] = raw_y_mg;
    accel_sum[1] += raw_y_mg;
    proc->accel_y_mg = accel_sum[1] / FILTER_SIZE;

    // --- Z AXIS ---
    accel_sum[2] -= accel_history[2][filter_idx];
    accel_history[2][filter_idx] = raw_z_mg;
    accel_sum[2] += raw_z_mg;
    proc->accel_z_mg = accel_sum[2] / FILTER_SIZE;

    // Increment index and wrap around (0 to 9)
    filter_idx++;
    if (filter_idx >= FILTER_SIZE)
    {
        filter_idx = 0;
    }

    proc->gyro_x_mdps = ((int32_t)(raw->gyro_x - proc->gyro_off_x) * 1000) / GYRO_SCALE_FACTOR;
    proc->gyro_y_mdps = ((int32_t)(raw->gyro_y - proc->gyro_off_y) * 1000) / GYRO_SCALE_FACTOR;
    proc->gyro_z_mdps = ((int32_t)(raw->gyro_z - proc->gyro_off_z) * 1000) / GYRO_SCALE_FACTOR;
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