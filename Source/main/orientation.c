
#include "orientation.h"
#include "nvs_flash.h"
#include <string.h>

float declinationAngle = 0; // todo setup to web interface http://www.magnetic-declination.com/

hmc5883l_dev_t dev_hmc5883l;

OrientationSensors orientation_inited = Orientation_none;

int16_t orientationXMin = INT16_MAX;
int16_t orientationYMin = INT16_MAX;
int16_t orientationZMin = INT16_MAX;
int16_t orientationXMax = INT16_MIN;
int16_t orientationYMax = INT16_MIN;
int16_t orientationZMax = INT16_MIN;

void reset_orientation_calibration()
{
    orientationXMin = INT16_MAX;
    orientationYMin = INT16_MAX;
    orientationZMin = INT16_MAX;
    orientationXMax = INT16_MIN;
    orientationYMax = INT16_MIN;
    orientationZMax = INT16_MIN;
}

void load_calibration()
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("orient", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        nvs_get_i16(nvs_handle, "orientationXMin", &orientationXMin);
        nvs_get_i16(nvs_handle, "orientationYMin", &orientationYMin);
        nvs_get_i16(nvs_handle, "orientationZMin", &orientationZMin);
        nvs_get_i16(nvs_handle, "orientationXMax", &orientationXMax);
        nvs_get_i16(nvs_handle, "orientationYMax", &orientationYMax);
        nvs_get_i16(nvs_handle, "orientationZMax", &orientationZMax);
        int16_t tmp = 0;
        nvs_get_i16(nvs_handle, "declination", &tmp);
        declinationAngle = tmp / 100; // 2 decimal precision
        nvs_close(nvs_handle);
    }
    else
    {
        reset_orientation_calibration();
    }
}

void save_calibration()
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("orient", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        nvs_set_i16(nvs_handle, "orientationXMin", orientationXMin);
        nvs_set_i16(nvs_handle, "orientationYMin", orientationYMin);
        nvs_set_i16(nvs_handle, "orientationZMin", orientationZMin);
        nvs_set_i16(nvs_handle, "orientationXMax", orientationXMax);
        nvs_set_i16(nvs_handle, "orientationYMax", orientationYMax);
        nvs_set_i16(nvs_handle, "orientationZMax", orientationZMax);
        nvs_set_i16(nvs_handle, "declination", (int16_t)(declinationAngle * 100));
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

void init_orientation()
{
    // HCM5883l
    memset(&dev_hmc5883l, 0, sizeof(hmc5883l_dev_t));
    hmc5883l_init_desc(&dev_hmc5883l, 0, 5, 4);

    if (hmc5883l_init(&dev_hmc5883l) == ESP_OK)
    {
        hmc5883l_set_opmode(&dev_hmc5883l, HMC5883L_MODE_CONTINUOUS);
        hmc5883l_set_samples_averaged(&dev_hmc5883l, HMC5883L_SAMPLES_8);
        hmc5883l_set_data_rate(&dev_hmc5883l, HMC5883L_DATA_RATE_07_50);
        hmc5883l_set_gain(&dev_hmc5883l, HMC5883L_GAIN_1090);
        ESP_LOGI("Orientation", "hmc5883lTestConnection OK");
        orientation_inited = Orientation_hmc5883l;
    }
    else
    {
        hmc5883l_free_desc(&dev_hmc5883l);
    }

    // end of list
    if (orientation_inited == Orientation_none)
    {
        ESP_LOGI("Orientation", "No compatible sensor found");
        return;
    }

    // load calibration data
    load_calibration();
}

float fix_heading(float heading)
{
    if (heading < 0)
        heading += 2 * M_PI;

    if (heading > 2 * M_PI)
        heading -= 2 * M_PI;
    return (heading + declinationAngle);
}

float get_heading()
{
    if (orientation_inited == Orientation_none)
        return 0;
    if (orientation_inited == Orientation_hmc5883l)
    {
        hmc5883l_data_t data;
        if (hmc5883l_get_data(&dev_hmc5883l, &data) == ESP_OK)
        {
            // printf("Magnetic data: X:%.2f mG, Y:%.2f mG, Z:%.2f mG\n", data.x, data.y, data.z);
            // todo use calibration
            return atan2(data.y, data.x);
        }
    }
    return 0;
}

float get_heading_degrees()
{
    if (orientation_inited == Orientation_none)
        return 400;
    return get_heading() * 180 / M_PI + declinationAngle;
}

void calibrate_orientation(uint8_t sec)
{
    // todo, run a calibration for N sec to get min-max. should be around 10

    save_calibration();
}

void set_declination(float declination)
{
    declinationAngle = declination;
    save_calibration();
}
float get_declination() { return declinationAngle; }