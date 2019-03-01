/*
 *
 * CSEE 4840 Lab 2 for 2019
 *
 * Name/UNI: Rui Chen/rc3205, Shaofu Wu/sw3385 
 */
#include "fbputchar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>

/* Update SERVER_HOST to be the IP address of
 * the chat server you are connecting to
 */
/* micro36.ee.columbia.edu */
#define SERVER_HOST "128.59.148.182"
#define SERVER_PORT 42000

#define BUFFER_SIZE 128
#define MAX_PER_ROW 64
#define INIT_ROW 22
#define INIT_COL 0
#define MAXROW 23
#define HIG_BOUND_FIR 0
#define LOW_BOUND_FIR 9
#define HIG_BOUND_SEC 11
#define LOW_BOUND_SEC 20
#define HIG_BOUND_THI 22
#define LOW_BOUND_THI 23

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int sockfd; /* Socket file descriptor */

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

pthread_t network_thread;
void *network_thread_f(void *);
void Custum_Initial();
void InitiateRow(int, int);
int JudgeClass(char);


int main()
{
  int err, col, currentCol, currentRow;
  char dispCharacter;
  char writeString[500];
  char *writeStringHead;
  char *writeHead;
  struct sockaddr_in serv_addr;
  int count=0;
  int i, flag;
  int currentIndex;
  struct usb_keyboard_packet packet;
  int transferred;
  char keystate[12];
  int row2=HIG_BOUND_SEC;
  char writeStringBuffer1[MAX_PER_ROW+1];
  char writeStringBuffer2[MAX_PER_ROW+1];
  char selfBuffer[10][MAX_PER_ROW+1];
  char memory=0;



  currentIndex = 0;
  if ((err = fbopen()) != 0) {
    fprintf(stderr, "Error: Could not open framebuffer: %d\n", err);
    exit(1);
  }
  /* initial the screen */
  Custum_Initial();
  /* Draw rows of asterisks across the top and bottom of the screen */
  for (col = 0 ; col < 64 ; col++) {
    fbputchar('*', 10, col);
    fbputchar('*',21,col);
  }
  
  /* Open the keyboard */
  if ( (keyboard = openkeyboard(&endpoint_address)) == NULL ) {
    fprintf(stderr, "Did not find a keyboard\n");
    exit(1);
  }
    
  /* Create a TCP communications socket */
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

  /* Get the server address */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  if ( inet_pton(AF_INET, SERVER_HOST, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Could not convert host IP \"%s\"\n", SERVER_HOST);
    exit(1);
  }

  /* Connect the socket to the server */
  if ( connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error: connect() failed.  Is the server running?\n");
    exit(1);
  }

  /* Start the network thread */
  pthread_create(&network_thread, NULL, network_thread_f, NULL);

  /* Look for and handle keypresses */
  currentRow=HIG_BOUND_THI;
  currentCol=INIT_COL;
  writeStringHead = writeString;
  writeHead = writeString;
  for (;;) {
    libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);
    if (transferred == sizeof(packet)) {
      sprintf(keystate, "%02x %02x %02x", packet.modifiers, packet.keycode[0],packet.keycode[1]);
      
      /* Here we find a key pressed*/

      if (memory<strlen(packet.keycode)){
        dispCharacter = keyValue(packet.modifiers,packet.keycode[strlen(packet.keycode)-1]);
         /* Do something until here*/
         /* Assume we have a function JudgeClass to judge whether it's a control or a letter, return flag=0 if it is a letter, flag!=0 if it is a function  */
        flag = JudgeClass(packet.keycode[0]);
         
         
         if (flag == 1){ /* if Enter is pressed */  
           /* Assume the string to write is less than BUFFER_SIZE */
           write(sockfd, writeString, strlen(writeString)); /* send to server*/
           
           /* whenever put in long string */
           /* check the length first */
            if (!strlen(writeString)) {
                continue;
            }
            
            else if (strlen(writeString)<=MAX_PER_ROW) {/* if length is shorter than a single line width */

                strcpy(selfBuffer[row2-HIG_BOUND_SEC],writeString);
                writeString[0] = '\0';
                fbputs(selfBuffer[row2-HIG_BOUND_SEC],row2,0);
                row2++;

                /* if it reaches to the boundary, then scroll */
                if (row2 > LOW_BOUND_SEC) {
                  InitiateRow(HIG_BOUND_SEC,LOW_BOUND_SEC);
                  row2 = LOW_BOUND_SEC;
                  for(i = 0; i < 9 ; i++)
                  {
                    strcpy(selfBuffer[i],selfBuffer[i+1]);
                    fbputs(selfBuffer[i],HIG_BOUND_SEC + i,0);
                  }
                  selfBuffer[9][0]='\0';
                }
                /* finish scroll */
            }
            
            else{/* if length is larger than one line, split them into two lines */

                strncpy(selfBuffer[row2-HIG_BOUND_SEC],writeString,MAX_PER_ROW);
                selfBuffer[row2-HIG_BOUND_SEC][MAX_PER_ROW]='\0';
                fbputs(selfBuffer[row2-HIG_BOUND_SEC],row2,0);
                row2++;
                /* if it reaches to the boundary, then scroll */
                if (row2 > LOW_BOUND_SEC) {
                  InitiateRow(HIG_BOUND_SEC,LOW_BOUND_SEC);
                  row2 = LOW_BOUND_SEC;
                  for(i = 0; i < 9 ; i++)
                  {
                    strcpy(selfBuffer[i],selfBuffer[i+1]);
                    fbputs(selfBuffer[i],HIG_BOUND_SEC + i,0);
                  }
                  selfBuffer[9][0]='\0';
                }
                /* finish scroll */

                strncpy(selfBuffer[row2-HIG_BOUND_SEC],writeString+MAX_PER_ROW,strlen(writeString)-MAX_PER_ROW);
                selfBuffer[row2-HIG_BOUND_SEC][strlen(writeString)-MAX_PER_ROW]='\0';
                fbputs(selfBuffer[row2-HIG_BOUND_SEC],row2,0);
                row2++;

                /* if it reaches to the boundary, then scroll */
                if (row2 > LOW_BOUND_SEC) {
                  InitiateRow(HIG_BOUND_SEC,LOW_BOUND_SEC);
                  row2 = LOW_BOUND_SEC;
                  for(i = 0; i < 9 ; i++)
                  {
                    strcpy(selfBuffer[i],selfBuffer[i+1]);
                    fbputs(selfBuffer[i],HIG_BOUND_SEC + i,0);
                  }
                  selfBuffer[9][0]='\0';
                }
                /* finish scroll */

                writeString[0] = '\0';
            }
            InitiateRow(HIG_BOUND_THI,LOW_BOUND_THI);
            count = 0;        /* counting the length of input string */
            currentCol = 0;   /* counting the positon of cursor */
            currentRow = HIG_BOUND_THI;
            fbputcursor(0x20,currentRow, currentCol);
         }


         else if (flag == 0){/* if a character is pressed */
           /* we only allow maximum 127 character, remain one position for cursor*/
            if (count<BUFFER_SIZE-1) {
	              /* Add the char to the middle of a string */
                for( i = strlen(writeString); i > currentIndex; i--){
                  writeString[i]=writeString[i-1];
                }
                writeString[i]=dispCharacter;
                count++;
                writeString[count]='\0';
                /* end of adding char to a string */
                if (currentCol==MAX_PER_ROW-1 && currentRow == HIG_BOUND_THI) {
                  currentCol = 0;
                  currentRow = LOW_BOUND_THI;
                }
                else
                {
                  currentCol++;
                }

	              printf("count2 = %d, col2= %d, row2= %d\n", count, currentCol, currentRow);
            }          
	       }
         else if (flag == 2) {/* if backspace is pressed */
           /*We delete the char at current index -1 and push chars behind it forward*/
            if (!(currentRow == HIG_BOUND_THI && currentCol == 0)) {
                /* remove some character from a string */
                for( i = currentIndex-1; i < strlen(writeString); i++){
                  writeString[i]=writeString[i+1];
                }
                /* finish remove a charactor from string */

                count--;
                if (currentCol == 0 && currentRow == LOW_BOUND_THI) {
                    currentRow = HIG_BOUND_THI;
                    currentCol = MAX_PER_ROW-1;
                }
                else
                {
                    currentCol--;
                }
            }
            
         }
         else if (flag == 3){/* if left direction key is pressed */
            /* Only work when it's not on the most left of first row */
            if (!(currentRow == HIG_BOUND_THI && currentCol == 0)) {
                if (currentCol == 0 && currentRow == LOW_BOUND_THI) {
                    currentRow = HIG_BOUND_THI;
                    currentCol = MAX_PER_ROW-1;
                }
                else
                {
                  currentCol--;
                }
            }
          }

          else if(flag == 6){/*handle unused keypressed*/
            continue;
          }
  
         else if (flag == 4) {/* if right direction key is pressed */
            /* Only work when it's not on the most right of second row */
            /* Only work when it's within the range of text */
            if (!(currentRow == LOW_BOUND_THI && currentCol == MAX_PER_ROW-1) && (currentIndex < count)) {
                if (currentRow == HIG_BOUND_THI && currentCol == MAX_PER_ROW-1 ) {
                    currentRow = LOW_BOUND_THI;
                    currentCol = 0;
                }
                else
                {
                    currentCol++;
                    printf("current Index is %d count is %d\n",currentIndex, count);
                }
            }
            else
            {
              continue;
            }
            
          }

         else if (flag == 5) {/* if delete is pressed */
           /*We delete the char at current index and push chars behind it forward*/
            if (!(currentRow == HIG_BOUND_THI && currentCol == 0) && currentIndex<count) {
                currentIndex = MAX_PER_ROW*(currentRow-HIG_BOUND_THI)+currentCol;

                
                /* remove some character from a string */
                for( i = currentIndex; i < strlen(writeString); i++){
                  writeString[i]=writeString[i+1];
                }
                /* finish remove a charactor from string */

                count--;

            }
          
          /* Now we show the whole message */
         }
         else if (packet.keycode[0] == 0x29) { /* ESC pressed? */
	        break;
        }

        InitiateRow(HIG_BOUND_THI,LOW_BOUND_THI);
        if (strlen(writeString)<=MAX_PER_ROW) {
            fbputs(writeString, HIG_BOUND_THI, 0);
        }
        else{
            strncpy(writeStringBuffer1,writeString,MAX_PER_ROW);
            writeStringBuffer1[MAX_PER_ROW]='\0';
           fbputs(writeStringBuffer1,HIG_BOUND_THI,0);
                  
           strncpy(writeStringBuffer2,writeString+64,strlen(writeString)-MAX_PER_ROW);
           writeStringBuffer2[strlen(writeString)-MAX_PER_ROW]='\0';
           fbputs(writeStringBuffer2,LOW_BOUND_THI,0);                          
         }

        currentIndex = MAX_PER_ROW*(currentRow-HIG_BOUND_THI)+currentCol;
        if (currentIndex == count) {
          fbputcursor(0x20, currentRow, currentCol);
        }
        else
        {
          fbputcursor(writeString[currentIndex], currentRow, currentCol);
        }
        
     }
  }
  memory = strlen(packet.keycode);
}

  /* Terminate the network thread */
  pthread_cancel(network_thread);

  /* Wait for the network thread to finish */
  pthread_join(network_thread, NULL);

  return 0;
}

void Custum_Initial()
{
  int col, row;
  for (row = 0; row < 24; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}

void InitiateRow(int min, int max){
  int col, row;
  for (row = min; row < max + 1; row++){
    for (col = 0 ; col < 64 ; col++) {
      fbputchar(' ', row, col);
    }
  }	
}

int JudgeClass(char dispCharacter){
  switch (dispCharacter){
    /*enter */
    case 0x28:
      return 1;
      break;
    case 0x58:
      return 1;
      break;
    /*back */
    case 0x2a:
      return 2;
      break;
    /* left*/
    case 0x50:
      return 3;
      break;
    /* right */
    case 0x4F:
      return 4;
      break;
    /* delete */
    case 0x4c:
      return 5;
      break;
    /*down*/
    case 0x51:
      return 6;
      break;
    /*up*/
    case 0x52:
      return 6;
      break;
    /*Caps Lock*/
    case 0x39:
      return 6;
      break;
    default:
      return 0;
  }
}


	void *network_thread_f(void *ignored)
	{
	  char recvBuf[BUFFER_SIZE];
	  char readBuffer[10][65];
	  int n;
	  int i;
	  int readRow=HIG_BOUND_FIR;
	  /* Read: Receive data */
	  /* Assume the length is smaller than BUFFER_SIZE*/
	  while ( (n = read(sockfd, &recvBuf, BUFFER_SIZE - 1)) > 0 ) {
	    recvBuf[n] = '\0';
	    printf("%s", recvBuf);

	    if (n<=MAX_PER_ROW) {/* if length is shorter than a single line width */
	      for(i=0; i<n; i++){
		      readBuffer[readRow-HIG_BOUND_FIR][i]=recvBuf[i];
	      }
	      fbputs(readBuffer[readRow-HIG_BOUND_FIR],readRow,0);
	      readRow++;

	       /* if it reaches to the boundary, then scroll */
	      if (readRow > LOW_BOUND_FIR) {
		      InitiateRow(HIG_BOUND_FIR,LOW_BOUND_FIR);
		      readRow = LOW_BOUND_FIR;
		      for(i = 0; i < 9 ; i++){
		      strcpy(readBuffer[i],readBuffer[i+1]);
		      fbputs(readBuffer[i],HIG_BOUND_FIR + i,0);
		  }
		  readBuffer[9][0]='\0';
	      }
			/* finish scroll */
	    }
	      else{/* if length is larger than one line, split them into two lines */
	      for(i=0; i<64; i++){
		      readBuffer[readRow-HIG_BOUND_FIR][i]=recvBuf[i];
	      }
		  readBuffer[readRow-HIG_BOUND_FIR][MAX_PER_ROW]='\0';
		  fbputs(readBuffer[readRow-HIG_BOUND_FIR],readRow,0);
		  readRow++;
		  /* if it reaches to the boundary, then scroll */
		  if (readRow > LOW_BOUND_FIR) {
		    InitiateRow(HIG_BOUND_FIR,LOW_BOUND_FIR);
		    readRow = LOW_BOUND_FIR;
		    for(i = 0; i < 9 ; i++){
		      strcpy(readBuffer[i],readBuffer[i+1]);
		      fbputs(readBuffer[i],HIG_BOUND_FIR + i,0);
		    }
		  readBuffer[9][0]='\0';
		  }
		  /* finish scroll */

		 for(i=0; i<64; i++){
		  readBuffer[readRow-HIG_BOUND_FIR][i]=recvBuf[i+64];
	      	 }
		  readBuffer[readRow-HIG_BOUND_FIR][strlen(recvBuf)-MAX_PER_ROW]='\0';
		  fbputs(readBuffer[readRow-HIG_BOUND_FIR],readRow,0);
		  readRow++;

		  /* if it reaches to the boundary, then scroll */
		  if (readRow > LOW_BOUND_FIR) {
		    InitiateRow(HIG_BOUND_FIR,LOW_BOUND_FIR);
		    readRow = LOW_BOUND_FIR;
		    for(i = 0; i < 9 ; i++){
		      strcpy(readBuffer[i],readBuffer[i+1]);
		      fbputs(readBuffer[i],HIG_BOUND_FIR + i,0);
		    }
		  readBuffer[9][0]='\0';
		  }
		  /* finish scroll */
	       }
	    }
	  return NULL;
	}

