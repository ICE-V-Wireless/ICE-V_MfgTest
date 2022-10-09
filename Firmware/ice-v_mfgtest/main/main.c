/*
 * main.c - top level
 * part of ICE-V Wireless Manufacturing Test firmware
 * 07-27-22 E. Brombaugh
*/

#include "main.h"
#include "ice.h"
#include "driver/gpio.h"
#include "rom/crc.h"
#include "spiffs.h"
#include "wifi.h"
#include "adc_c3.h"
#include "mfgtst.h"
#include "esp32c3/rom/miniz.h"

#define BOOT_PIN 9
#define LED_PIN 10

static const char* TAG = "main";

/* build version in simple format */
const char *fwVersionStr = "V0.1";
const char *zlib_spi_file = "/spiffs/spi_pass.zlib";
const char *spi_file = "/spiffs/spi_pass.bin";
const char *mfg_file = "/spiffs/mfgtst.bin";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/* canonical size of a UP5k bitstream */
#define DATASIZE 104090

/*
 * Notes on compression:
 * ESP32C3 has the miniz zlib support built in to ROM and it's accessible via the
 * esp32c3/rom/miniz.h header. There are a variety of low/mid/high level functions
 * available for both compression and decompression and the header file has decent
 * info on using them.
 * To offline compress files in the spiffs directory use this command:
 * zlib-flate -compress < bitstream.bin > bitstream.zlib
 * (zlib-flate is part of the qpdf - tools for and transforming and inspecting PDF files
 * package)
 * This almost works, but calling the decompression functions seems to fail without
 * generating any useful debug info and usually results in a badly corrupted flash
 * that needs to be erased several times before reflashing from scratch.
 */

/*
 * configure with error and timeout
 */
uint32_t fpga_cfg(const char *file, uint8_t zip)
{
	uint8_t *bin = NULL, *unz = NULL;
	uint32_t sz;
	size_t unz_sz;
	uint32_t failcnt = 5, errcnt = 0;
	
	if(!spiffs_read((char *)file, &bin, &sz))
	{
		uint8_t cfg_stat;
		
		/* unzip? */
		if(zip)
		{
			printf("Decompressing %d bytes...\r\n", sz);
						
#if 0
			/* High-level decompress crashes */
			unz = tinfl_decompress_mem_to_heap(bin, sz, &unz_sz, TINFL_FLAG_PARSE_ZLIB_HEADER);
			if(!unz)
			{
				ESP_LOGW(TAG, "Decompress failed.");
				return 1;
			}
#else
			/* mid-level decompress requires pre-allocating memory */
			unz = malloc(DATASIZE);
			if(!unz)
			{
				ESP_LOGW(TAG, "Couldn't allocate memory for decompressing.");
				free(bin);
				return 1;
			}
			
			if((unz_sz = tinfl_decompress_mem_to_mem(unz, DATASIZE, bin, sz, TINFL_FLAG_PARSE_ZLIB_HEADER)) == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED)
			{
				ESP_LOGW(TAG, "Decompress failed.");
				free(unz);
				free(bin);
				return 1;
			}
			else
				ESP_LOGI(TAG, "Decompress successful.");
#endif
			sz = unz_sz;
		}
		else
		{
			unz = bin;
		}
		
		/* loop on config failure */
		while((cfg_stat = ICE_FPGA_Config(unz, sz)) && (failcnt--))
			ESP_LOGW(TAG, "FPGA configure FAILED - status = %d. Retrying", cfg_stat);
		
		if(failcnt)
			ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
		else
		{
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
			errcnt++;
		}
		
		/* did we unzip */
		if(zip)
			free(unz);
		
		free(bin);
	}
	
	return errcnt;
}

/*
 * entry point
 */
void app_main(void)
{
	uint32_t errcnt = 0;
	
	/* Startup */
    ESP_LOGI(TAG, "-----------------------------");
    ESP_LOGI(TAG, "ICE-V Wireless Manufacturing Test starting...");
    ESP_LOGI(TAG, "Version: %s", fwVersionStr);
    ESP_LOGI(TAG, "Build Date: %s", bdate);
    ESP_LOGI(TAG, "Build Time: %s", btime);

	if(spiffs_init() == ESP_OK)
		ESP_LOGI(TAG, "#TEST# SPIFFS Initialize PASS");
	else
	{
		ESP_LOGW(TAG, "#TEST# SPIFFS Initialize FAIL");
		errcnt++;
	}
	
	/* init FPGA SPI port */
	if(ICE_Init() == ESP_OK)
		ESP_LOGI(TAG, "#TEST# FPGA SPI port initialize PASS");
	else
	{
		ESP_LOGW(TAG, "#TEST# FPGA SPI port initialize FAIL");
		errcnt++;
	}
	
	/* configure FPGA from SPIFFS file for SPRAM Test */
	if(!fpga_cfg(spi_file, 0))
		ESP_LOGI(TAG, "#TEST# FPGA Pass-Thru Config PASS");
	else
	{
		ESP_LOGW(TAG, "#TEST# FPGA Pass-Thru Config FAIL");
		errcnt++;
	}
	
	/* do PSRAM tests */
#define TST_BUFSZ 32768
	{
		uint32_t i;
		
		/* buffer for PSRAM testing */
		uint8_t	*testbuf = malloc(TST_BUFSZ);
				
		/* fill it with a count sequence */
		for(i=0;i<TST_BUFSZ;i++)
			testbuf[i] = (i&0xff);
				
		/* write to PSRAM */
		ICE_PSRAM_Write(0, testbuf, TST_BUFSZ);
		ESP_LOGI(TAG, "Wrote %d bytes", TST_BUFSZ);
		
		/* read from PSRAM */
		ICE_PSRAM_Read(0, testbuf, TST_BUFSZ);
		ESP_LOGI(TAG, "Read %d bytes", TST_BUFSZ);
		
		/* Check for match */
		uint32_t errs = 0;
		for(i=0;i<TST_BUFSZ;i++)
		{
			if(testbuf[i] != (i&0xff))
				errs++;
		}
		ESP_LOGI(TAG, "Detected %d mismatches in PSRAM read data", errs);
		
		if(!errs)
			ESP_LOGI(TAG, "#TEST# PSRAM Test PASS");
		else
		{
			ESP_LOGW(TAG, "#TEST# PSRAM Test FAIL");
			errcnt++;
		}
		
		free(testbuf);
	}

	/* configure FPGA from SPIFFS file for Mfg Test */
	if(!fpga_cfg(mfg_file, 0))
		ESP_LOGI(TAG, "#TEST# FPGA Mfg Test Config PASS");
	else
	{
		ESP_LOGW(TAG, "#TEST# FPGA Mfg Test Config FAIL");
		errcnt++;
	}
	
	/* check for expected ID register */
	uint32_t Data;
	ICE_FPGA_Serial_Read(MFGTST_REG_ID, &Data);
	if(Data == MFGTST_ID)
		ESP_LOGI(TAG, "#TEST# FPGA bitstream ID PASS");
	else
    {
		ESP_LOGW(TAG, "#TEST# FPGA bitstream ID FAIL");
		errcnt++;
    }
	
	/* Check Clk counter */
	ICE_FPGA_Serial_Read(MFGTST_REG_CLK, &Data);
    ESP_LOGI(TAG, "Clock test register = %d", Data);
	if((Data > 4570) && (Data < 5053))
		ESP_LOGI(TAG, "#TEST# Clock Test PASS");
	else
    {
		ESP_LOGW(TAG, "#TEST# Clock Test FAIL");
		errcnt++;
    }
		
	/* Check PMOD I/O */
    ESP_LOGI(TAG, "Checking PMOD I/O");
	uint8_t pmod_err = 0;
	for(uint8_t type = 0;type < 2; type++)
	{
		uint32_t sel_bit, pmod_out, pmod_in;
		
		for(uint bit = 0;bit < 12; bit++)
		{
			/* create walking 1/0 */
			sel_bit = (1<<bit);
			pmod_out = type ? sel_bit : ~sel_bit;
			pmod_out &= 0xFFF;
			
			/* Write & readback */
			ICE_FPGA_Serial_Write(MFGTST_REG_GPO, pmod_out);
			ICE_FPGA_Serial_Read(MFGTST_REG_GPI, &pmod_in);
			//printf("%03X : %03X\n", pmod_out, pmod_in);
	
			/* mask */
			pmod_out &= sel_bit;
			pmod_in &= sel_bit;
			
			/* test */
			if(pmod_in != pmod_out)
			{
				uint8_t bits = (bit%4)*2 + 1;
				ESP_LOGW(TAG, "PMOD %d bits %d-%d fail @ val %d (%03X ~ %03X)",
					bit/4+1, bits, (bits+3)&7, type, pmod_out, pmod_in);
				pmod_err++;
			}				
		}
	}
	ESP_LOGI(TAG, "Detected %d errors in PMOD test", pmod_err);
	if(!pmod_err)
		ESP_LOGI(TAG, "#TEST# PMOD Continuity Test PASS");
	else
    {
		ESP_LOGW(TAG, "#TEST# PMOD Continuity Test FAIL");
		errcnt++;
    }
	
	/* Test GPIO 2 & 8 by driving from FPGA P34 (gpo reg bit 12) */
	gpio_set_direction(2, GPIO_MODE_INPUT);
	gpio_set_direction(8, GPIO_MODE_INPUT);
	pmod_err = 0;
	for(uint8_t type = 0;type < 2; type++)
	{
		ICE_FPGA_Serial_Write(MFGTST_REG_GPO, (type << 12));
		vTaskDelay(1);
		if(gpio_get_level(2)!= type)
		{
			ESP_LOGW(TAG, "GPIO2 input test failed for value %d", type);
			pmod_err++;
		}
		if(gpio_get_level(8)!= type)
		{
			ESP_LOGW(TAG, "GPIO8 input test failed for value %d", type);
			pmod_err++;
		}
	}
	ICE_FPGA_Serial_Write(MFGTST_REG_GPO, 0);
	
	/* Test GPIO 20 & 21 connectivity */
	gpio_reset_pin(20);	// get 'em out of UART mode
	gpio_reset_pin(21);
	gpio_set_direction(20, GPIO_MODE_INPUT);	// RX pin
	gpio_set_direction(21, GPIO_MODE_OUTPUT);	// TX pin
	for(uint8_t type = 0;type < 2; type++)
	{
		gpio_set_level(21, type);
		vTaskDelay(1);
		if(gpio_get_level(20)!= type)
		{
			ESP_LOGW(TAG, "GPIO20/21 test failed for value %d", type);
			pmod_err++;
		}
	}
	ESP_LOGI(TAG, "Detected %d errors in C3 GPIO test", pmod_err);
	
	if(!pmod_err)
		ESP_LOGI(TAG, "#TEST# C3 GPIO Test PASS");
	else
    {
		ESP_LOGW(TAG, "#TEST# C3 GPIO Test FAIL");
		errcnt++;
    }
	
    /* Test BOOT button */
	/* set up BOOT button GPIO */
	gpio_set_direction(BOOT_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BOOT_PIN, GPIO_PULLUP_ONLY);
	
	/* wait for boot button released */
	ESP_LOGI(TAG, "Waiting for Boot button release.");
	uint32_t timeout = 0, boot_btn_err = 0;
	vTaskDelay(1);
#define BOOT_BTN_TIMEOUT 1000
	while((gpio_get_level(BOOT_PIN)==0) && (timeout++ < BOOT_BTN_TIMEOUT))
	{
		vTaskDelay(1);
	}
	if(timeout >= BOOT_BTN_TIMEOUT)
    {
		ESP_LOGW(TAG, "Timeout waiting for Boot Button Release.");
		boot_btn_err++;
    }
	else
	{
		ESP_LOGI(TAG, "Boot button released.");
	}
		
	/* wait for boot button pressed */
	ICE_FPGA_Serial_Write(2, 0x808080);	// white LED
	ESP_LOGI(TAG, "#TEST# ----Press Boot Button Now----");
	timeout = 0;
	while((gpio_get_level(BOOT_PIN)==1) && (timeout++ < BOOT_BTN_TIMEOUT))
	{
		vTaskDelay(1);
	}
	if(timeout >= BOOT_BTN_TIMEOUT)	// 10 second timeout
    {
		ESP_LOGW(TAG, "Timeout waiting for Boot Button Pressed.");
		boot_btn_err++;
    }
	else
	{
		ESP_LOGI(TAG, "Boot Button Pressed.");
	}
	ICE_FPGA_Serial_Write(2, 0);	// LED Off
	if(!boot_btn_err)
		ESP_LOGI(TAG, "#TEST# Boot Button Test PASS");
	else
    {
		ESP_LOGW(TAG, "#TEST# Boot Button Test FAIL");
		errcnt++;
    }

    /* init ADC for Vbat readings */
    if(!adc_c3_init())
    {
		ESP_LOGI(TAG, "ADC Initialized");
		int16_t i, vbat[128];
		int64_t sampletime[128];
		
		/* acquire samples and stats */
		int32_t mean = 0, min = 8192, max = 0;
		for(i=0;i<128;i++)
		{
			vbat[i] = adc_c3_get();
			mean += vbat[i];
			min = min > vbat[i] ? vbat[i] : min;
			max = max < vbat[i] ? vbat[i] : max;
			sampletime[i] = esp_timer_get_time();
			vTaskDelay(1);
		}
		
		/* compute mean */
		mean /= 128;
		ESP_LOGI(TAG, "|Vbat| = %d mV, min = %d mV, max = %d mV", 2*mean, 2*min, 2*max);
		
		/* dump samples and search for edges */
		int64_t pt = 0, dt;
		for(i=0;i<128;i++)
		{
			//printf("% 4d %8lld ", 2*vbat[i], sampletime[i]);
			if((i>1) && (vbat[i-1] < mean) && (vbat[i] >= mean) && ((vbat[i] - vbat[i-1])>20))
			{
				dt = sampletime[i] - pt;
				pt = sampletime[i];
				//printf("%8lld %8lld ", dt, pt);
			}
			//printf("\n");
		}
		
		/* compute period */
		float per, frq;
		per = 1e-6 * dt;
		frq = 1.0F/per;
		ESP_LOGI(TAG, "Period = %f sec (%f Hz)", per, frq);
		
		/* check bounds */
		if((2*max > 4100) && (2*max < 4300) && (frq > 1.0f) && (frq < 3.0f))
		{
			ESP_LOGI(TAG, "#TEST# Charge Test PASS");
		}
		else
		{
			ESP_LOGW(TAG, "#TEST# Charge Test FAIL");
			errcnt++;
		}
    }
	else
    {
		ESP_LOGW(TAG, "#TEST# ADC Init Failed");
		errcnt++;
    }
	
	/* init WiFi & socket */
	if(!wifi_init())
	{
		ESP_LOGI(TAG, "#TEST# WiFi PASS");
		ESP_LOGI(TAG, "RSSI: %d", wifi_get_rssi());
	}
	else
	{
		ESP_LOGW(TAG, "#TEST# WiFi FAIL");
		errcnt++;
    }
	
	/* Done */
	ESP_LOGI(TAG, "%d Errors Detected", errcnt);
	if(!errcnt)
		ESP_LOGI(TAG, "#TEST# Complete PASS");
	else
		ESP_LOGW(TAG, "#TEST# Complete FAIL");
	
	/* wait here forever and blink */
    ESP_LOGI(TAG, "Looping...", btime);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	uint8_t i = 0;
	while(1)
	{
		/* toggle C3 LED */
		gpio_set_level(LED_PIN, i&1);
		
		/* cycle RGB LED on FPGA */
		Data = 0x80 << ((i%3)*8);
		ICE_FPGA_Serial_Write(2, Data);
		
		i++;
		
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
