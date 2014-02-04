#define _WIN32_WINNT 0x501
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "acn.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <winsock2.h>
#include <winsock.h>

#define SERIAL 1

const uint8_t raw_acn_packet[sizeof(struct E131_2009)] = {
#include "acnraw.h"
};

struct E131_2009 packet;

WSADATA wsaData;
SOCKET acnSock;
boolean changed = FALSE;
SOCKADDR_IN brdcastaddr;
SOCKADDR_IN bindaddr;
HANDLE m_hCommPort;

#define FPS 44 // How many sACN frames per second

DWORD WINAPI senderThread(LPVOID lpdwThreadParam)
{
  int tick;

  tick = 0;
  for (;;)
  {
    Sleep(1000/FPS);
    tick++;

    if (tick < FPS && !changed)
      continue;

    tick = 0;
    changed = FALSE;

    packet.seq_num[0]++;

    sendto(acnSock, (char*)&packet, sizeof(packet), 0, (SOCKADDR*)&brdcastaddr, sizeof(brdcastaddr));
  }
}

DWORD WINAPI parserThread(LPVOID lpdwThreadParam)
{
  while (consoleparse())
  {
    printf("consoleparse()\n");
  }
}

int run(char *szPortName)
{
  printf("Hello, World:  size is %d\n", sizeof(raw_acn_packet));

  int iResult;

  memcpy(&packet, raw_acn_packet, sizeof(packet));
  memset(packet.dmx_data, 0, sizeof(packet.dmx_data));

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
      printf("WSAStartup failed: %d\n", iResult);
      return 1;
  }

  //char* quack_addr = "239.255.0.1";
  char* quack_addr = "127.0.0.1";
  int quack_port = 5568;

  bindaddr.sin_family = AF_INET;
  //bindaddr.sin_addr.s_addr = inet_addr("192.168.1.22");
  brdcastaddr.sin_addr.s_addr = INADDR_ANY;
  bindaddr.sin_port = 0;

  brdcastaddr.sin_family = AF_INET;
  brdcastaddr.sin_addr.s_addr = inet_addr(quack_addr);
  //brdcastaddr.sin_addr.s_addr = INADDR_BROADCAST;
  brdcastaddr.sin_port = htons(quack_port);

  acnSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  printf("Socket = %d\n", acnSock);

  packet.universe[1] = 1;

  char opt = 1;
  setsockopt(acnSock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));

  bind(acnSock, (SOCKADDR*)&bindaddr, sizeof(bindaddr));

#ifdef SERIAL
  m_hCommPort = CreateFile(szPortName,
	GENERIC_READ|GENERIC_WRITE,
	0,    //(share) 0:cannot share the COM port                        
	0,    //security  (None)                
	OPEN_EXISTING,// creation : open_existing
	0,// we want overlapped operation
	0// no templates file for COM port...
  );

  DCB deviceControlBlock;

  deviceControlBlock.DCBlength = sizeof(deviceControlBlock);
  GetCommState(m_hCommPort, &deviceControlBlock);
  deviceControlBlock.BaudRate = CBR_115200;
  deviceControlBlock.StopBits = ONESTOPBIT;
  deviceControlBlock.Parity   = NOPARITY;
  deviceControlBlock.ByteSize = DATABITS_8;
  deviceControlBlock.fRtsControl = 0;
  SetCommState(m_hCommPort, &deviceControlBlock);

  printf("Serial Port = %d\n", m_hCommPort);
#endif

  CreateThread(NULL, 0, parserThread, NULL, 0, NULL); 
  CreateThread(NULL, 0, senderThread, NULL, 0, NULL); 
}

consoleerror(char *msg)
{
  fprintf(stderr, "Msg: %s\n", msg);
}

#if SERIAL
char buffer[1];
DWORD read = 0;
DWORD buffptr = 0;
#endif

/*
 * Fetch the next input charactor from the console.
 */
int consolelex()
{
#if SERIAL
  OVERLAPPED ov;
  int z;

  if (buffptr <= read)
  {
    z = ReadFile(m_hCommPort, buffer, sizeof(buffer), &read, NULL);
    // fwrite(buffer, read, 1, stdout);
    buffptr = 0;
  }

  return buffer[buffptr++];

#else
  return getc(stdin);
#endif
}

int ma2lex()
{
  return 0;
}

int ma2error()
{
}

/*
 */
void setOutput(int channel, int value)
{
  packet.dmx_data[channel-1] = (uint8_t) value;
  changed = TRUE;
}

void mouseUp(int amount)
{
}

void mouseDown(int amount)
{
}

void mouseX(int amount)
{
}

void mouseY(int amount)
{
}
