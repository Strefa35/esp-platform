/**
 * @file types.h
 * @author A.Czerwinski@pistacje.net
 * @brief 
 * @version 0.1
 * @date 2025-04-06
 * 
 * @copyright Copyright (c) 2025 4Embedded.Systems
 * 
 */

#ifndef __TYPES_H__
#define __TYPES_H__


#define GET_OP_TYPE_NAME(_op) ( \
  _op == OP_TYPE_SET        ? "OP_TYPE_SET"         : \
  _op == OP_TYPE_GET        ? "OP_TYPE_GET"         : \
  _op == OP_TYPE_RECEIVE    ? "OP_TYPE_RECEIVE"     : \
  _op == OP_TYPE_EVENT      ? "OP_TYPE_EVENT"       : \
                              "OP_TYPE_UNKNOWN"       \
)


/*
==================================================================
  ENUMS
==================================================================
*/

/* Message type definition */
typedef enum {
  OP_TYPE_UNKNOWN,

  OP_TYPE_SET,
  OP_TYPE_GET,
  OP_TYPE_RECEIVE,
  OP_TYPE_EVENT,

} operation_type_e;


operation_type_e convertOperation(const char* op_str) {
  operation_type_e op = OP_TYPE_UNKNOWN;

  if (op_str) {
    if (strcmp(op_str, "set") == 0) {
      op = OP_TYPE_SET;
    } else if (strcmp(op_str, "get") == 0) {
      op = OP_TYPE_GET;
    } else if (strcmp(op_str, "receive") == 0) {
      op = OP_TYPE_RECEIVE;
    } else if (strcmp(op_str, "event") == 0) {
      op = OP_TYPE_EVENT;
    }
  }
  return op;
}

#endif /* __TYPES_H__ */
