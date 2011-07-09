/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "MsgParser.hpp"
#include "Checksum.hpp"

#define IMICOMM_MAX_MSG_SIZE (sizeof(TMsg))

namespace IMI {
namespace MessageParser {
  /**
   * @brief Parser state
   */
  enum TState {
    STATE_NOT_SYNC,                               /**< @brief Synchronization bits not found */
    STATE_COMM_MSG                                /**< @brief Parsing message body */
  };

  TState _state;                                  /**< @brief Parser state */
  IMIBYTE _msgBuffer[IMICOMM_MAX_MSG_SIZE];       /**< @brief Parsed message buffer */
  unsigned _msgBufferPos;                         /**< @brief Current position in a message buffer */
  unsigned _msgBytesLeft;                         /**< @brief Remaining number of bytes of the message to parse */

  /**
   * Cast the head of the buffer to a TMsg.
   */
  TMsg &GetMessage();
  bool Check(const TMsg *msg, IMIDWORD size);
}
}

IMI::TMsg &
IMI::MessageParser::GetMessage()
{
  return *(TMsg *)(void *)_msgBuffer;
}

/**
 * @brief Resets the state of the parser
 */
void IMI::MessageParser::Reset()
{
  _msgBytesLeft = 0;
  _msgBufferPos = 0;
  _state = STATE_NOT_SYNC;
}


/**
 * @brief Verifies received message
 *
 * @param msg Message to check
 * @param size Size of received message
 *
 * @return Verification status
 */
bool IMI::MessageParser::Check(const TMsg *msg, IMIDWORD size)
{
  // minimal size of comm message
  if(size < IMICOMM_MSG_HEADER_SIZE + IMICOMM_CRC_LEN)
    return false;

  // check signature
  if(msg->syncChar1 != IMICOMM_SYNC_CHAR1 || msg->syncChar2 != IMICOMM_SYNC_CHAR2)
    return false;

  // check size
  if(msg->payloadSize != size - IMICOMM_MSG_HEADER_SIZE - IMICOMM_CRC_LEN)
    return false;

  // check CRC
  IMIWORD crc1 = CRC16Checksum(((IMIBYTE*)msg) + IMICOMM_SYNC_LEN, IMICOMM_MSG_HEADER_SIZE + msg->payloadSize - IMICOMM_SYNC_LEN);
  IMIWORD crc2 = (IMIWORD)(((IMIBYTE*)msg)[size - 1]) | ((IMIWORD)(((IMIBYTE*)msg)[size - 2]) << 8);
  if(crc1 != crc2)
    return false;

  return true;
}


/**
 * @brief Parses received message chunk
 *
 * @param buffer Buffer with received data
 * @param size The size of received data
 *
 * @return Received message or 0 if invalid on incomplete.
 */
const IMI::TMsg *IMI::MessageParser::Parse(const IMIBYTE buffer[], int size)
{
  const IMIBYTE *ptr = buffer;
  const TMsg *msg = 0;

  for(;size; size--) {
    IMIBYTE byte = *ptr++;

    if(_state == STATE_NOT_SYNC) {
      // verify synchronization chars
      if(byte == IMICOMM_SYNC_CHAR1 && _msgBufferPos == 0) {
        _msgBuffer[_msgBufferPos++] = byte;
      }
      else if(byte == IMICOMM_SYNC_CHAR2 && _msgBufferPos == 1) {
        _msgBuffer[_msgBufferPos++] = byte;
        _state = STATE_COMM_MSG;
      }
      else {
        _msgBufferPos = 0;
      }
    }
    else if(_state == STATE_COMM_MSG) {
      if(_msgBufferPos < IMICOMM_MSG_HEADER_SIZE) {
        // copy header
        _msgBuffer[_msgBufferPos++] = byte;
      }
      else {
        if(_msgBufferPos == IMICOMM_MSG_HEADER_SIZE) {
          // verify payload size
          _msgBytesLeft = GetMessage().payloadSize + IMICOMM_CRC_LEN;
          if(_msgBytesLeft > COMM_MAX_PAYLOAD_SIZE + IMICOMM_CRC_LEN) {
            // Invalid length
            Reset();
            continue;
          }
        }

        // copy payload
        _msgBytesLeft--;
        if(_msgBufferPos < sizeof(_msgBuffer)) // Just in case
          _msgBuffer[_msgBufferPos++] = byte;

        if(_msgBytesLeft == 0) {
          // end of message
          if(Check(&GetMessage(), _msgBufferPos))
            msg = &GetMessage();

          // prepare parser for the next message
          Reset();
        }
      }
    }
  }

  return msg;
}
