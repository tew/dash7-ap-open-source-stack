/*! \file alp.h
 *

 *  \copyright (C) Copyright 2015 University of Antwerp and others (http://oss-7.cosys.be)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \author glenn.ergeerts@uantwerpen.be
 * \author maarten.weyn@uantwerpen.be
 *
 */

/*! \file alp.h
 * \addtogroup ALP
 * \ingroup D7AP
 * @{
 * \brief Application Layer Protocol APIs
 * \author	glenn.ergeerts@aloxy.be
 */

#ifndef ALP_H_
#define ALP_H_

#include "stdint.h"
#include "stdbool.h"
#include "modules_defs.h"

#ifdef MODULE_D7AP
#include "d7ap.h"
#endif

#ifdef MODULE_LORAWAN
#include "lorawan_stack.h"
#endif

#include "d7ap_fs.h"
#include "dae.h"

#include "fifo.h"

#define MODULE_ALP_INTERFACE_SIZE 10

#define ALP_PAYLOAD_MAX_SIZE D7A_PAYLOAD_MAX_SIZE // TODO configurable?
#define ALP_ITF_CONFIG_SIZE 43

typedef enum
{
    ALP_ITF_ID_HOST = 0x00,
    ALP_ITF_ID_SERIAL = 0x01, // not part of the spec
    ALP_ITF_ID_LORAWAN_ABP = 0x02, // not part of the spec
    ALP_ITF_ID_LORAWAN_OTAA = 0x03, // not part of the spec
    ALP_ITF_ID_D7ASP = 0xD7
} alp_itf_id_t;


typedef enum {
    ALP_OP_NOP = 0,
    ALP_OP_READ_FILE_DATA = 1,
    ALP_OP_READ_FILE_PROPERTIES = 2,
    ALP_OP_WRITE_FILE_DATA = 4,
    ALP_OP_WRITE_FILE_DATA_FLUSH = 5,
    ALP_OP_WRITE_FILE_PROPERTIES = 6,
    ALP_OP_ACTION_QUERY = 8,
    ALP_OP_BREAK_QUERY = 9,
    ALP_OP_PERMISSION_REQUEST = 10,
    ALP_OP_VERIFY_CHECKSUM = 11,
    ALP_OP_EXIST_FILE = 16,
    ALP_OP_CREATE_FILE = 17,
    ALP_OP_DELETE_FILE = 18,
    ALP_OP_RESTORE_FILE = 19,
    ALP_OP_FLUSH_FILE = 20,
    ALP_OP_OPEN_FILE = 21,
    ALP_OP_CLOSE_FILE = 22,
    ALP_OP_COPY_FILE = 23,
    ALP_OP_EXECUTE_FILE = 31,
    ALP_OP_RETURN_FILE_DATA = 32,
    ALP_OP_RETURN_FILE_PROPERTIES = 33,
    ALP_OP_STATUS = 34,
    ALP_OP_RESPONSE_TAG = 35,
    ALP_OP_CHUNK = 48,
    ALP_OP_LOGIC = 49,
    ALP_OP_FORWARD = 50,
    ALP_OP_INDIRECT_FORWARD = 51,
    ALP_OP_REQUEST_TAG = 52
} alp_operation_t;

// define the (max) size for all ALP operation types
#define ALP_OP_SIZE_REQUEST_TAG (1 + 1)
#define ALP_OP_SIZE_READ_FILE_DATA (1 + 5 + 4)

typedef enum {
  ALP_STATUS_OK = 0x00,
  ALP_STATUS_PARTIALLY_COMPLETED = 0x01,
  ALP_STATUS_UNKNOWN_ERROR = 0x80,
  ALP_STATUS_UNKNOWN_OPERATION = 0xF6,
  ALP_STATUS_INSUFFICIENT_PERMISSIONS = 0xFC,
  // TODO others
  ALP_STATUS_INCOMPLETE_OPERAND = 0xF5,
  ALP_STATUS_FILE_ID_ALREADY_EXISTS = 0xFE,
  ALP_STATUS_FILE_ID_NOT_EXISTS = 0xFF,
} alp_status_codes_t;

typedef enum {
  ARITH_COMP_TYPE_INEQUALITY = 0,
  ARITH_COMP_TYPE_EQUALITY = 1,
  ARITH_COMP_TYPE_LESS_THAN = 2,
  ARITH_COMP_TYPE_LESS_THAN_OR_EQUAL_TO = 3,
  ARITH_COMP_TYPE_GREATER_THAN = 4,
  ARITH_COMP_TYPE_GREATER_THAN_OR_EQUAL_TO = 5
} alp_query_arithmetic_comparison_type_t;

typedef struct {
    uint8_t itf_id;
    union {
        // 'known' interfaces can use the typed variable (which will be serialized when necessary), other interfaces need to fill the raw buffer
        uint8_t itf_config[ALP_ITF_CONFIG_SIZE];
#ifdef MODULE_D7AP
        d7ap_session_config_t d7ap_session_config;
#endif
#ifdef MODULE_LORAWAN
        lorawan_session_config_otaa_t lorawan_session_config_otaa;
        lorawan_session_config_abp_t lorawan_session_config_abp;
#endif
    };
} alp_interface_config_t;


/*! \brief The ALP CTRL header
 *
 * note: bit order is important here since this is send over the air. We explicitly reverse the order to ensure BE.
 * Although bit fields can cause portability problems it seems fine for now using gcc and the current platforms.
 * If this poses problems in the future we must resort to bit arithmetics and flags.
 */
typedef struct {
    union {
        uint8_t raw;
        struct {
            alp_operation_t operation : 6;
            bool b6 : 1;
            bool b7 : 1;
        };
    };
} alp_control_t;

/*! \brief The ALP CTRL header, for 'regular' operations, where b6 and b7 are overloaded with response_requested and group flags respectively
 */
typedef struct {
    union {
        uint8_t raw;
        struct {
            alp_operation_t operation : 6;
            bool response_requested : 1;
            bool group : 1;
        };
    };
} alp_control_regular_t;

/*! \brief The ALP CTRL header, for tag request operation
 */
typedef struct {
    union {
        uint8_t raw;
        struct {
            alp_operation_t operation : 6;
            bool _rfu : 1;
            bool respond_when_completed : 1;
        };
    };
} alp_control_tag_request_t;

/*! \brief The ALP CTRL header, for tag response operation
 */
typedef struct {
    union {
        uint8_t raw;
        struct {
            alp_operation_t operation : 6;
            bool error : 1;
            bool _rfu : 1;
        };
    };
} alp_control_tag_response_t;

typedef struct {
    uint8_t file_id;
    uint32_t offset;
} alp_operand_file_offset_t;

typedef struct {
    alp_operand_file_offset_t file_offset;
    uint32_t requested_data_length;
} alp_operand_file_data_request_t;

typedef struct {
    alp_operand_file_offset_t file_offset;
    uint32_t provided_data_length;
    uint8_t data[255]; // TODO fixed size?
} alp_operand_file_data_t;

typedef struct {
    uint8_t file_id;
    d7ap_fs_file_header_t file_header;
} alp_operand_file_header_t;

typedef struct {
    alp_itf_id_t itf_id;
    uint8_t len;
    union {
      uint8_t itf_status[40];
#ifdef MODULE_D7AP
      d7ap_session_result_t d7ap_session_result;
#endif
#ifdef MODULE_LORAWAN
      lorawan_session_result_t lorawan_session_result;
#endif
    };
} alp_interface_status_t;


typedef struct {
    alp_operation_t operation;
    union {
        alp_operand_file_data_t file_data_operand;
        struct {
            bool completed;
            bool error;
            uint8_t tag_id;
        } tag_response;
        alp_interface_status_t status;
    };

} alp_action_t;

typedef void (*interface_deinit)();

typedef struct {
    alp_itf_id_t itf_id;
    uint8_t itf_cfg_len;
    uint8_t itf_status_len;
    error_t (*send_command)(uint8_t* payload, uint8_t payload_length, uint8_t expected_response_length, uint16_t* trans_id, alp_interface_config_t* itf_cfg);
    void (*init)(alp_interface_config_t* itf_cfg);
    interface_deinit deinit;
    bool unique; // TODO
} alp_interface_t;


/*!
 * \brief Returns the ALP operation type contained in alp_command
 * \param alp_command
 * \return the ALP operation type
 */
alp_operation_t alp_get_operation(uint8_t* alp_command);


uint8_t alp_get_expected_response_length(fifo_t fifo);

alp_status_codes_t alp_register_interface(alp_interface_t* itf);
void alp_append_tag_request_action(fifo_t* fifo, uint8_t tag_id, bool eop);
void alp_append_read_file_data_action(fifo_t* fifo, uint8_t file_id, uint32_t offset, uint32_t length, bool resp, bool group);
void alp_append_write_file_data_action(fifo_t* fifo, uint8_t file_id, uint32_t offset, uint32_t length, uint8_t* data, bool resp, bool group);
void alp_append_forward_action(fifo_t* fifo, alp_interface_config_t* config, uint8_t config_len);
void alp_append_return_file_data_action(fifo_t* fifo, uint8_t file_id, uint32_t offset, uint32_t length, uint8_t* data);
void alp_append_length_operand(fifo_t* fifo, uint32_t length);
void alp_append_create_new_file_data_action(fifo_t* fifo, uint8_t file_id, uint32_t length, fs_storage_class_t storage_class, bool resp, bool group);
void alp_append_indirect_forward_action(fifo_t* fifo, uint8_t file_id, bool overload, uint8_t *overload_config, uint8_t overload_config_len);
void alp_append_interface_status(fifo_t* fifo, alp_interface_status_t* status);

uint32_t alp_parse_length_operand(fifo_t* cmd_fifo);
alp_operand_file_offset_t alp_parse_file_offset_operand(fifo_t* cmd_fifo);
alp_operand_file_header_t alp_parse_file_header_operand(fifo_t* cmd_fifo);

void alp_parse_action(fifo_t* fifo, alp_action_t* action);

uint8_t alp_length_operand_coded_length(uint32_t length);

#endif /* ALP_H_ */

/** @}*/
