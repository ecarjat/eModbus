#include "ModbusTCP.h"

// Constructor takes reference to Client (EthernetClient or WiFiClient)
ModbusTCP::ModbusTCP(Client& client, uint16_t queueLimit) :
  PhysicalInterface(2000),
  MT_client(client),
  MT_lastHost(IPAddress(0, 0, 0, 0)),
  MT_lastPort(0),
  MT_targetHost(IPAddress(0, 0, 0, 0)),
  MT_targetPort(0),
  MT_qLimit(queueLimit),
  MT_sameHostInterval(50) { }

// Alternative Constructor takes reference to Client (EthernetClient or WiFiClient) plus initial target host
ModbusTCP::ModbusTCP(Client& client, IPAddress host, uint16_t port, uint16_t queueLimit) :
  PhysicalInterface(2000),
  MT_client(client),
  MT_lastHost(IPAddress(0, 0, 0, 0)),
  MT_lastPort(0),
  MT_targetHost(host),
  MT_targetPort(port),
  MT_qLimit(queueLimit),
  MT_sameHostInterval(50) { }

// Destructor: clean up queue, task etc.
ModbusTCP::~ModbusTCP() {
  // Clean up queue
  {
    // Safely lock access
    lock_guard<mutex> lockGuard(qLock);
    // Get all queue entries one by one
    while (!requests.empty()) {
      // Pull front entry
      TCPRequest *r = requests.front();
      // Delete request
      delete r;
      // Remove front entry
      requests.pop();
    }
  }
  // Kill task
  vTaskDelete(worker);
}

// begin: start worker task
void ModbusTCP::begin(int coreID) {
  // Create unique task name
  char taskName[12];
  snprintf(taskName, 12, "Modbus%02XTCP", instanceCounter);
  // Start task to handle the queue
  xTaskCreatePinnedToCore((TaskFunction_t)&handleConnection, taskName, 4096, this, 5, &worker, coreID >= 0 ? coreID : NULL);
}

// Switch target host (if necessary)
// Return true, if host/port is different from last host/port used
bool ModbusTCP::setTarget(IPAddress host, uint16_t port) {
  MT_targetHost = host;
  MT_targetPort = port;
  if (MT_targetHost == MT_lastHost && MT_targetPort == MT_lastPort) return false;
  return true;
}

// Set pause time between two consecutive requests to the same target host
void ModbusTCP::setSameHostInterval(uint32_t intervalMS) {
  MT_sameHostInterval = intervalMS;
}

// Methods to set up requests
// 1. no additional parameter (FCs 0x07, 0x0b, 0x0c, 0x11)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}

vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}

// 2. one uint16_t parameter (FC 0x18)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, p1, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t p1) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, p1);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}
  
// 3. two uint16_t parameters (FC 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, p1, p2, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, p1, p2);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}

// 4. three uint16_t parameters (FC 0x16)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint16_t p3, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, p1, p2, p3, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint16_t p3) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, p1, p2, p3);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}
  
// 5. two uint16_t parameters, a uint8_t length byte and a uint8_t* pointer to array of words (FC 0x10)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint16_t *arrayOfWords, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, p1, p2, count, arrayOfWords, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint16_t *arrayOfWords) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, p1, p2, count, arrayOfWords);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}
  
// 6. two uint16_t parameters, a uint8_t length byte and a uint16_t* pointer to array of bytes (FC 0x0f)
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint8_t *arrayOfBytes, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, p1, p2, count, arrayOfBytes, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint8_t *arrayOfBytes) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, p1, p2, count, arrayOfBytes);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}

// 7. generic constructor for preformatted data ==> count is counting bytes!
Error ModbusTCP::addRequest(uint8_t serverID, uint8_t functionCode, uint16_t count, uint8_t *arrayOfBytes, uint32_t token) {
  Error rc = SUCCESS;        // Return value

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, MT_targetHost, MT_targetPort, serverID, functionCode, count, arrayOfBytes, token);

  // Add it to the queue, if valid
  if (r) {
    r->tcpHead.transactionID = messageCount++;
    // Queue add successful?
    if (!addToQueue(r)) {
      // No. Return error after deleting the allocated request.
      rc = REQUEST_QUEUE_FULL;
      delete r;
    }
  }

  return rc;
}
  
vector<uint8_t> ModbusTCP::generateRequest(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, uint16_t count, uint8_t *arrayOfBytes) {
  Error rc = SUCCESS;       // Return code from generating the request
  vector<uint8_t> rv;       // Returned std::vector with the message or error code

  // Create request, if valid
  TCPRequest *r = TCPRequest::createTCPRequest(rc, 99, 99, serverID, functionCode, count, arrayOfBytes);

  // Put it in the return std::vector
  rv = vectorize(transactionID, r, rc);
  
  // Delete request again, if one was created
  if (r) delete r;

  // Move back vector contents
  return rv;
}

// addToQueue: send freshly created request to queue
bool ModbusTCP::addToQueue(TCPRequest *request) {
  bool rc = false;
  // Did we get one?
  if (request) {
    if(requests.size()<MT_qLimit) {
      // Yes. Safely lock queue and push request to queue
      rc = true;
      lock_guard<mutex> lockGuard(qLock);
      requests.push(request);
    }
  }

  return rc;
}

  // Move complete message data including tcpHead into a std::vector
  vector<uint8_t> ModbusTCP::vectorize(uint16_t transactionID, TCPRequest *request, Error err) {
    vector<uint8_t> rv;       /// Returned std::vector

    // Was the message generated?
    if (err != SUCCESS) {
      // No. Return the Error code only - vector size is 1
      rv.reserve(1);
      rv.push_back(err);
    }
    // If it was successful - did we get a message?
    else if (request) {
      // Yes, obviously. 
      // Resize the vector to take tcpHead (6 bytes) + message proper
      rv.reserve(request->len() + 6);
      rv.resize(request->len() + 6);

      // Do a fast (non-C++-...) copy
      uint8_t *cp = rv.data();
      // Copy in TCP header
      makeHead(cp, 6, transactionID, request->tcpHead.protocolID, request->tcpHead.len);
      // Copy in message contents
      memcpy(cp + 6, request->data(), request->len());
    }
    // Bring back the vector
    return rv;
  }

// Method to generate an error response - properly enveloped for TCP
vector<uint8_t> ModbusTCP::generateErrorResponse(uint16_t transactionID, uint8_t serverID, uint8_t functionCode, Error errorCode) {
  vector<uint8_t> rv;       // Returned std::vector
  
  rv.reserve(9);            // 6 bytes TCP header plus serverID, functionCode and error code
  rv.resize(9);

  // Copy in TCP header
  uint8_t *cp = rv.data();
  makeHead(cp, 6, transactionID, 0, 3);

  // Add payload
  rv[6] = serverID;
  rv[7] = (functionCode | 0x80);
  rv[8] = errorCode;

  return rv;
}

// handleConnection: worker task
// This was created in begin() to handle the queue entries
void ModbusTCP::handleConnection(ModbusTCP *instance) {
  const uint8_t RETRIES(2);
  uint8_t retryCounter = RETRIES;
  bool doNotPop;
  uint32_t lastRequest = millis();

  // Loop forever - or until task is killed
  while (1) {
    // Do we have a request in queue?
    if (!instance->requests.empty()) {
      // Yes. pull it.
      TCPRequest *request = instance->requests.front();
      doNotPop = false;

      // onGenerate handler registered?
      if (instance->onGenerate) {
        // Yes. Send request packet
        instance->onGenerate("Request ", request->data(), request->len(), request->getToken());
      }
      // Empty the RX buffer - just in case...
      while (instance->MT_client.available()) instance->MT_client.read();

      // check if lastHost/lastPort!=host/port off the queued request
      if (instance->MT_lastHost != request->targetHost || instance->MT_lastPort != request->targetPort) {
        // It is different. If client is connected, disconnect
        if (instance->MT_client.connected()) {
          // Is connected - cut it
          instance->MT_client.stop();
          delay(1);  // Give scheduler room to breathe
        }
      }
      else {
        // it is the same host/port. Give it some slack to get ready again
        while (millis() - lastRequest < instance->MT_sameHostInterval) {
          delay(1);
        }
      }
      // if client is disconnected (we will have to switch hosts)
      if (!instance->MT_client.connected()) {
        // It is disconnected. connect to host/port from queue
        instance->MT_client.connect(request->targetHost, request->targetPort);

        delay(1);  // Give scheduler room to breathe
      }
      // Are we connected (again)?
      if (instance->MT_client.connected()) {
        // Yes. Send the request via IP
        instance->send(request);

        // Get the response - if any
        TCPResponse *response = instance->receive(request);

        // onGenerate handler registered?
        if (instance->onGenerate) {
          // Yes. Send request packet
          instance->onGenerate("Response ", response->data(), response->len(), request->getToken());
        }
        // Did we get a normal response?
        if (response->getError()==SUCCESS) {
          // Yes. Do we have an onData handler registered?
          if(instance->onData) {
            // Yes. call it
            instance->onData(response->getServerID(), response->getFunctionCode(), response->data(), response->len(), request->getToken());
          }
        }
        else {
          // No, something went wrong. All we have is an error
          if (response->getError() == TIMEOUT && retryCounter--) {
            Serial.println("Retry on timeout...");
            doNotPop = true;
          }
          else {
            // Do we have an onError handler?
            if(instance->onError) {
              // Yes. Forward the error code to it
              instance->onError(response->getError(), request->getToken());
            }
          }
        }
        //   set lastHost/lastPort tp host/port
        instance->MT_lastHost = request->targetHost;
        instance->MT_lastPort = request->targetPort;
        delete response;  // object created in receive()
      }
      else {
        // Oops. Connection failed
        // Retry, if attempts are left or report error.
        if (retryCounter--) {
          instance->MT_client.stop();
          delay(10);
          Serial.println("Retry on connect failure...");
          doNotPop = true;
        }
        else {
          // Do we have an onError handler?
          if(instance->onError) {
            // Yes. Forward the error code to it
            instance->onError(IP_CONNECTION_FAILED, request->getToken());
          }
        }
      }
      // Clean-up time. 
      if (!doNotPop)
      {
        // Safely lock the queue
        lock_guard<mutex> lockGuard(instance->qLock);
        // Remove the front queue entry
        instance->requests.pop();
        retryCounter = RETRIES;
        // Delete request
        delete request;   // object created from addRequest()
      }
      lastRequest = millis();
    }
    else {
      delay(1);  // Give scheduler room to breathe
    }
  }
}

// makeHead: helper function to set up a MSB TCP header
bool ModbusTCP::makeHead(uint8_t *data, uint16_t dataLen, uint16_t TID, uint16_t PID, uint16_t LEN) {
  uint16_t headlong = 6;
  uint16_t offs = 0;
  uint16_t ptr = 0;

  if (dataLen < headlong) return false;   // Will not fit
  if (data == nullptr) return false;      // No data allocated?

  offs = ModbusMessage::addValue(data + ptr, headlong, TID);
  headlong -= offs;
  ptr += offs;
  offs = ModbusMessage::addValue(data + ptr, headlong, PID);
  headlong -= offs;
  ptr += offs;
  offs = ModbusMessage::addValue(data + ptr, headlong, LEN);
  headlong -= offs;
  // headlong should be 0 here!
  if(headlong) return false;
  return true;
}

// send: send request via Client connection
void ModbusTCP::send(TCPRequest *request) {
  // We have a established connection here, so we can write right away.
  // Wait...: tcpHead is not yet in MSB order:
  uint8_t head[6];
  if (makeHead(head, 6, request->tcpHead.transactionID, request->tcpHead.protocolID, request->tcpHead.len)) {
    // Write TCP header first
    MT_client.write(head, 6);
    // Request comes next
    MT_client.write(request->data(), request->len());
    // Done. Are we?
    MT_client.flush();
  }
  // else????
}

// receive: get response via Client connection
TCPResponse* ModbusTCP::receive(TCPRequest *request) {
  uint32_t lastMillis = millis();     // Timer to check for timeout
  bool hadData = false;               // flag data received
  const uint16_t dataLen(300);          // Modbus Packet supposedly will fit (260<300)
  uint8_t data[dataLen];              // Local buffer to collect received data
  uint16_t dataPtr = 0;               // Pointer into data
  TCPResponse *response = nullptr;    // Response structure to be returned

  // wait for packet data, overflow or timeout
  while (millis() - lastMillis < timeOutValue && dataPtr < dataLen && !hadData) {
    // Is there data waiting?
    if (MT_client.available()) {
      // Yes. catch as much as is there and fits into buffer
      while (MT_client.available() && dataPtr < dataLen) {
        data[dataPtr++] = MT_client.read();
      }
      // Register data received
      hadData = true;
      // Rewind EOT and timeout timers
      lastMillis = millis();
    }
    delay(1); // Give scheduler room to breathe
  }
  // Did we get some data?
  if (hadData) {
    // Yes. check it for validity
    uint8_t head[6];
    makeHead(head, 6, request->tcpHead.transactionID, request->tcpHead.protocolID, dataPtr - 6);
    // First transactionID and protocolID shall be identical, Are they?
    if(memcmp(head, data, 6)) {
      // No. return Error response
      response = errorResponse(TCP_HEAD_MISMATCH, request);
    }
    else {
      // Looks good.
      response = new TCPResponse(dataPtr - 6);
      response->add(dataPtr - 6, data + 6);
      response->tcpHead.transactionID = request->tcpHead.transactionID;
      response->tcpHead.protocolID = request->tcpHead.protocolID;
      response->tcpHead.len = dataPtr - 6;
    }
  }
  else {
    // No, timeout must have struck
    response = errorResponse(TIMEOUT, request);
  }
  return response;
}

TCPResponse* ModbusTCP::errorResponse(Error e, TCPRequest *request) {
  TCPResponse *errResponse = new TCPResponse(3);
  
  errResponse->add(request->getServerID());
  errResponse->add(static_cast<uint8_t>(request->getFunctionCode() | 0x80));
  errResponse->add(static_cast<uint8_t>(e));
  errResponse->tcpHead.transactionID = request->tcpHead.transactionID;
  errResponse->tcpHead.protocolID = request->tcpHead.protocolID;
  errResponse->tcpHead.len = 3;

  return errResponse;
}