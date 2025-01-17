#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"
#include "logging.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) 
    return ERR_CODE_NULL_POINTER;

  RETURN_IF_ERROR_CODE(writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode));

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  error_code_t errCode;
  
  // First check that temp isn't NULL
  if (temp == NULL)
    return ERR_CODE_NULL_POINTER;

  // Buffer to send commands and receive data from LM758D
  uint8_t buf[2] = {0};

  // 16 bit integer to help with celsius conversion
  uint16_t conversion = 0;

  // Send first byte of buffer which is 0x00 to select temperature register
  RETURN_IF_ERROR_CODE(i2cSendTo(devAddr, buf, 1)); 

  // Receive 2 bytes and store them in buffer
  RETURN_IF_ERROR_CODE(i2cReceiveFrom(devAddr, buf, 2));

  // Check if D10 (bit 7) of MSB is a 1 to see if we calculate twos complement 
  if (buf[0] >> 7){
    conversion = ((buf[0] << 3) + (buf[1] >> 5));
    *temp = -1 * ((conversion ^ 0X7FF) + 1); // XOR with 11 1s to get 1s complement and then add 1 for 2s complement
  } else {
    *temp = ((buf[0] << 3) + (buf[1] >> 5));
  }
  
  *temp *= 0.125;

  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}
