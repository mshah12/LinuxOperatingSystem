#ifndef RTC_H
#define RTC_H
//ports and registers for RTC
#define CMOSRW70  0x70
#define REGNUM71  0x71
#define regA	  0x8A
#define regB	  0x8B
#define regC 	  0x0C
#define IRQ8 	  8
#define MAXRATE 32768
#define MINFREQ   2
#define MAXFREQ   1024

//init functions
void start_rtc_periodic_interrupt();
void rtc_init();

//driver functions
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd,const void* buf, int32_t nbytes);

//helper functions
// to set frequency
void rtc_freq_set(int32_t frq);
//to check is power of two or not
int32_t powtwo(int32_t frq);


unsigned int interruptcount;
extern unsigned int int_seen;
#endif /* RTC_H */
