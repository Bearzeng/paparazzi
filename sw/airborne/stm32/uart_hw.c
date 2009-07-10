#include "uart.h"

#include <stm32/rcc.h>
#include <stm32/misc.h>
#include <stm32/usart.h>
#include <stm32/gpio.h>
#include "std.h"

#ifdef USE_UART3 

uint16_t uart3_rx_insert_idx, uart3_rx_extract_idx;
uint8_t  uart3_rx_buffer[UART3_RX_BUFFER_SIZE];
uint8_t  uart3_tx_buffer[UART3_TX_BUFFER_SIZE];
volatile uint16_t uart3_tx_insert_idx, uart3_tx_extract_idx;
volatile bool_t uart3_tx_running;


void uart3_init( void ) {
  /* init RCC */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

  /* Enable USART3 interrupts */
  NVIC_InitTypeDef nvic;
  nvic.NVIC_IRQChannel = USART3_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 0;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  /* Init GPIOS */
  GPIO_InitTypeDef gpio;
  /* GPIOB: USART3 Tx push-pull */
  gpio.GPIO_Pin   = GPIO_Pin_10;
  gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &gpio);
  /* GPIOB: USART3 Rx pin as floating input */
  gpio.GPIO_Pin   = GPIO_Pin_11;
  gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &gpio);

  /* Configure USART3 */
  USART_InitTypeDef usart;
  usart.USART_BaudRate            = UART3_BAUD;
  usart.USART_WordLength          = USART_WordLength_8b;
  usart.USART_StopBits            = USART_StopBits_1;
  usart.USART_Parity              = USART_Parity_No;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART3, &usart);
  /* Enable USART3 Receive interrupts */
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
  /* Enable the USART3 */
  USART_Cmd(USART3, ENABLE);

  // initialize the transmit data queue
  uart3_tx_extract_idx = 0;
  uart3_tx_insert_idx = 0;
  uart3_tx_running = FALSE;

  // initialize the receive data queue
  uart3_rx_extract_idx = 0;
  uart3_rx_insert_idx = 0;

}

void uart3_transmit( uint8_t data ) {

  uint16_t temp = (uart3_tx_insert_idx + 1) % UART3_TX_BUFFER_SIZE;

  if (temp == uart3_tx_extract_idx)
    return;                          // no room

  USART_ITConfig(USART3, USART_IT_TXE, DISABLE);

  // check if in process of sending data
  if (uart3_tx_running) { // yes, add to queue
    uart3_tx_buffer[uart3_tx_insert_idx] = data;
    uart3_tx_insert_idx = temp;
  }
  else { // no, set running flag and write to output register
    uart3_tx_running = TRUE;
    USART_SendData(USART3, data);
  }

  USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

}

bool_t uart3_check_free_space( uint8_t len) {
  int16_t space = uart3_tx_extract_idx - uart3_tx_insert_idx;
  if (space <= 0)
    space += UART3_TX_BUFFER_SIZE;
  return (uint16_t)(space - 1) >= len;
}


void usart3_irq_handler(void) {
  
  if(USART_GetITStatus(USART3, USART_IT_TXE) != RESET){
    // check if more data to send
    if (uart3_tx_insert_idx != uart3_tx_extract_idx) {
      USART_SendData(USART3,uart3_tx_buffer[uart3_tx_extract_idx]);
      uart3_tx_extract_idx++;
      uart3_tx_extract_idx %= UART3_TX_BUFFER_SIZE;
    }
    else {
      uart3_tx_running = 0;       // clear running flag
      USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    }
  }

  if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET){
    uint16_t temp = (uart3_rx_insert_idx + 1) % UART3_RX_BUFFER_SIZE;;
    uart3_rx_buffer[uart3_rx_insert_idx] = USART_ReceiveData(USART3);
    // check for more room in queue
    if (temp != uart3_rx_extract_idx)
      uart3_rx_insert_idx = temp; // update insert index
  }

}


#endif /* USE_UART3 */
