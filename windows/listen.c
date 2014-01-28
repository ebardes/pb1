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

const uint8_t raw_acn_packet[sizeof(struct E131_2009)] = {
#include "acnraw.h"
};

struct E131_2009 packet;

WSADATA wsaData;
SOCKET acnSock;
boolean changed = FALSE;
SOCKADDR_IN brdcastaddr;

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
  while (yyparse())
  {
    printf("yyparse()\n");
  }
}

int main(int argc, char **argv)
{
  printf("Hello, World:  size is %d\n", sizeof(raw_acn_packet));

  int iResult;
  char *szPortName = argv[1];

  memcpy(&packet, raw_acn_packet, sizeof(packet));

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
      printf("WSAStartup failed: %d\n", iResult);
      return 1;
  }

  char* quack_addr = "239.255.0.10";
  int quack_port = 5568;

  brdcastaddr.sin_family = AF_INET;
  brdcastaddr.sin_addr.s_addr = inet_addr(quack_addr);
  //brdcastaddr.sin_addr.s_addr = INADDR_BROADCAST;
  brdcastaddr.sin_port = htons(quack_port);

  acnSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  printf("Socket = %d\n", acnSock);

  packet.universe[1] = 10;

  char opt = 1;
  setsockopt(acnSock, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));

#ifdef SERIAL
  HANDLE m_hCommPort = CreateFile(szPortName, GENERIC_READ|GENERIC_WRITE,
	0,    //(share) 0:cannot share the COM port                        
	0,    //security  (None)                
	OPEN_EXISTING,// creation : open_existing
	FILE_FLAG_OVERLAPPED,// we want overlapped operation
	0// no templates file for COM port...
  );

  printf("Serial Port = %d\n", m_hCommPort);
#endif

  CreateThread(NULL, 0, parserThread, NULL, 0, NULL); 
  // CreateThread(NULL, 0, senderThread, NULL, 0, NULL); 
  senderThread(NULL);
}

yyerror(char *msg)
{
  fprintf(stderr, "Msg: %s\n", msg);
}

int yylex()
{
  return getc(stdin);
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
