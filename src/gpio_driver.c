#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <asm/io.h>
#include <asm/uaccess.h>

// timer interval defined as (TIMER_SEC + TIMER_NANO_SEC)
#define TIMER_SEC    0
#define TIMER_NANO_SEC  1000*1000 /* 1ms */

/* GPIO registers base address. */
#define BCM2708_PERI_BASE   (0x3F000000)
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)
#define GPIO_ADDR_SPACE_LEN (0xB4)

//Handle GPIO: 0-9 GPIO Function Select 0.
#define GPFSEL0_OFFSET (0x00000000)
//Handle GPIO: 10-19 GPIO Function Select 1.
#define GPFSEL1_OFFSET (0x00000004)
//Handle GPIO: 20-29 GPIO Function Select 2.
#define GPFSEL2_OFFSET (0x00000008)
//Handle GPIO: 30-39 GPIO Function Select 3.
#define GPFSEL3_OFFSET (0x0000000C)
//Handle GPIO: 40-49 GPIO Function Select 4.
#define GPFSEL4_OFFSET (0x00000010)
//Handle GPIO: 50-53 GPIO Function Select 5.
#define GPFSEL5_OFFSET (0x00000014)
//GPIO: 0-31 GPIO Pin Output Set 0.
#define GPSET0_OFFSET (0x0000001C)
//GPIO: 32-53 GPIO Pin Output Set 1.
#define GPSET1_OFFSET (0x00000020)
//GPIO: 0-31 GPIO Pin Output Clear 0.
#define GPCLR0_OFFSET (0x00000028)
//GPIO: 32-53 GPIO Pin Output Clear 1.
#define GPCLR1_OFFSET (0x0000002C)
//GPIO: 0-31 GPIO Pin Level 0.
#define GPLEV0_OFFSET (0x00000034)
//GPIO: 32-53 GPIO Pin Level 1.
#define GPLEV1_OFFSET (0x00000038)
//GPIO: 0-53 GPIO Pin Pull-up/down Enable.
#define GPPUD_OFFSET (0x00000094)
//GPIO: 0-31 GPIO Pull-up/down Clock Register 0.
#define GPPUDCLK0_OFFSET (0x00000098)
//GPIO: 32-53 GPIO Pull-up/down Clock Register 1.
#define GPPUDCLK1_OFFSET (0x0000009C)

typedef enum {PULL_NONE = 0, PULL_DOWN = 1, PULL_UP = 2} PUD;
typedef enum {GPIO_DIRECTION_IN = 0, GPIO_DIRECTION_OUT = 1} DIRECTION;

/* GPIO pins available on connector p1. */
#define GPIO_02 (2)
#define GPIO_03 (3)
#define GPIO_04 (4)
#define GPIO_05 (5)
#define GPIO_06 (6)
#define GPIO_07 (7)
#define GPIO_08 (8)
#define GPIO_09 (9)
#define GPIO_10 (10)
#define GPIO_11 (11)
#define GPIO_12 (12)
#define GPIO_13 (13)
#define GPIO_14 (14)
#define GPIO_15 (15)
#define GPIO_16 (16)
#define GPIO_17 (17)
#define GPIO_18 (18)
#define GPIO_19 (19)
#define GPIO_20 (20)
#define GPIO_21 (21)
#define GPIO_22 (22)
#define GPIO_23 (23)
#define GPIO_24 (24)
#define GPIO_25 (25)
#define GPIO_26 (26)
#define GPIO_27 (27)

/* Declaration of gpio_driver.c functions */
int gpio_driver_init(void);
void gpio_driver_exit(void);
static int gpio_driver_open(struct inode *, struct file *);
static int gpio_driver_release(struct inode *, struct file *);
static ssize_t gpio_driver_read(struct file *, char *buf, size_t , loff_t *);
static ssize_t gpio_driver_write(struct file *, const char *buf, size_t , loff_t *);

/* Structure that declares the usual file access functions. */
struct file_operations gpio_driver_fops =
{
    open    :   gpio_driver_open,
    release :   gpio_driver_release,
    read    :   gpio_driver_read,
    write   :   gpio_driver_write
};

/* Declaration of the init and exit functions. */
module_init(gpio_driver_init);
module_exit(gpio_driver_exit);

/* Major number. */
int gpio_driver_major;
/* Buffer to store data. */
#define BUF_LEN 80
char* gpio_driver_buffer;
/* Blink timer vars. */
static struct hrtimer pwm_timer;
static ktime_t kt;

/* Virtual address where the physical GPIO address is mapped */
void* virt_gpio_base;

unsigned int GetGPFSELReg(char pin)
{
    unsigned int addr;
    if(pin >= 0 && pin <10)
        addr = GPFSEL0_OFFSET;
    else if(pin >= 10 && pin <20)
        addr = GPFSEL1_OFFSET;
    else if(pin >= 20 && pin <30)
        addr = GPFSEL2_OFFSET;
    else if(pin >= 30 && pin <40)
        addr = GPFSEL3_OFFSET;
    else if(pin >= 40 && pin <50)
        addr = GPFSEL4_OFFSET;
    else /*if(pin >= 50 && pin <53) */
        addr = GPFSEL5_OFFSET;
  return addr;
}

char GetGPIOPinOffset(char pin)
{
    if(pin >= 0 && pin <10)
        pin = pin;
    else if(pin >= 10 && pin <20)
        pin -= 10;
    else if(pin >= 20 && pin <30)
        pin -= 20;
    else if(pin >= 30 && pin <40)
        pin -= 30;
    else if(pin >= 40 && pin <50)
        pin -= 40;
    else /*if(pin >= 50 && pin <53) */
        pin -= 50;
    return pin;
}

void SetInternalPullUpDown(char pin, PUD pull)
{
    unsigned int gppud_offset;
    unsigned int gppudclk_offset;
    unsigned int tmp;
    unsigned int mask;

    gppud_offset = GPPUD_OFFSET;
    gppudclk_offset = (pin < 32) ? GPPUDCLK0_OFFSET : GPPUDCLK1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    iowrite32(pull, virt_gpio_base + gppud_offset);
    tmp = ioread32(virt_gpio_base + gppudclk_offset);
    mask = 0x1 << pin;
    tmp |= mask;
    iowrite32(tmp, virt_gpio_base + gppudclk_offset);
    iowrite32(PULL_NONE, virt_gpio_base + gppud_offset);
    tmp = ioread32(virt_gpio_base + gppudclk_offset);
    mask = 0x1 << pin;
    tmp &= (~mask);
    iowrite32(tmp, virt_gpio_base + gppudclk_offset);
}

void SetGpioPinDirection(char pin, DIRECTION direction)
{
    unsigned int GPFSELReg_offset;
    unsigned int tmp;
    unsigned int mask;

    GPFSELReg_offset = GetGPFSELReg(pin);
    pin = GetGPIOPinOffset(pin);
    tmp = ioread32(virt_gpio_base + GPFSELReg_offset);
    if(direction) { //set as output: set 1
      mask = 0x1 << (pin*3);
      tmp |= mask;
    }
    else { //set as input: set 0
      mask = ~(0x1 << (pin*3));
      tmp &= mask;
    }
    iowrite32(tmp, virt_gpio_base + GPFSELReg_offset);
}

void SetGpioPin(char pin)
{
    unsigned int GPSETreg_offset;
    unsigned int tmp;
    
    GPSETreg_offset = (pin < 32) ? GPSET0_OFFSET : GPSET1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    tmp = 0x1 << pin;
    iowrite32(tmp, virt_gpio_base + GPSETreg_offset);
}

void ClearGpioPin(char pin)
{
    unsigned int GPCLRreg_offset;
    unsigned int tmp;
    
    GPCLRreg_offset = (pin < 32) ? GPCLR0_OFFSET : GPCLR1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    tmp = 0x1 << pin;
    iowrite32(tmp, virt_gpio_base + GPCLRreg_offset);
}

char GetGpioPinValue(char pin)
{
    unsigned int GPLEVreg_offset;
    unsigned int tmp;
    unsigned int mask;

    GPLEVreg_offset = (pin < 32) ?  GPLEV0_OFFSET : GPLEV1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    tmp = ioread32(virt_gpio_base + GPLEVreg_offset);
    mask = 0x1 << pin;
    tmp &= mask;
    return (tmp >> pin);
}

/* timer callback function called each time the timer expires
   flashes the LED0, reads the SW0 and prints its value to kernel log */
static enum hrtimer_restart pwm_timer_callback(struct hrtimer *param) {
    static char power = 0x0;
    static char gpio_12_val;

    power ^= 0x1;
    if (power)
        SetGpioPin(GPIO_06);
    else
        ClearGpioPin(GPIO_06);
    gpio_12_val = GetGpioPinValue(GPIO_12);
    printk(KERN_INFO "gpio_driver: gpio12 value: %d\n", gpio_12_val);

    hrtimer_forward(&pwm_timer, ktime_get(), kt);
    return HRTIMER_RESTART;
}

int gpio_driver_init(void){
    int result = -1;
    printk(KERN_INFO "Inserting gpio_driver module\n");
    result = register_chrdev(0, "gpio_driver", &gpio_driver_fops);
    if (result < 0) {
        printk(KERN_INFO "gpio_driver: cannot obtain major number %d\n", gpio_driver_major);
        return result;
    }
    gpio_driver_major = result;
    printk(KERN_INFO "gpio_driver major number is %d\n", gpio_driver_major);
    gpio_driver_buffer = kmalloc(BUF_LEN, GFP_KERNEL);
    if (!gpio_driver_buffer) {
        result = -ENOMEM;
        goto fail_no_mem;
    }
    memset(gpio_driver_buffer, 0, BUF_LEN);
    virt_gpio_base = ioremap(GPIO_BASE, GPIO_ADDR_SPACE_LEN);
    if(!virt_gpio_base)
    {
        result = -ENOMEM;
        goto fail_no_virt_mem;
    }
    SetGpioPinDirection(GPIO_06, GPIO_DIRECTION_OUT);
    SetInternalPullUpDown(GPIO_12, PULL_UP);
    SetGpioPinDirection(GPIO_12, GPIO_DIRECTION_IN);
    hrtimer_init(&pwm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    kt = ktime_set(TIMER_SEC, TIMER_NANO_SEC);
    pwm_timer.function = &pwm_timer_callback;
    hrtimer_start(&pwm_timer, kt, HRTIMER_MODE_REL);

    return 0;

fail_no_virt_mem:
    if (gpio_driver_buffer)
    {
        kfree(gpio_driver_buffer);
    }
fail_no_mem:
    unregister_chrdev(gpio_driver_major, "gpio_driver");

    return result;
}

void gpio_driver_exit(void){
    printk(KERN_INFO "Removing gpio_driver module\n");
    hrtimer_cancel(&pwm_timer);
    ClearGpioPin(GPIO_06);
    SetGpioPinDirection(GPIO_06, GPIO_DIRECTION_IN);
    SetInternalPullUpDown(GPIO_12, PULL_NONE);
    if (virt_gpio_base) {
        iounmap(virt_gpio_base);
    }
    if (gpio_driver_buffer) {
        kfree(gpio_driver_buffer);
    }
    unregister_chrdev(gpio_driver_major, "gpio_driver");
}

static int gpio_driver_open(struct inode *inode, struct file *filp){
    return 0;
}

static int gpio_driver_release(struct inode *inode, struct file *filp){
    return 0;
}

static ssize_t gpio_driver_read(struct file *filp, char *buf, size_t len, loff_t *f_pos){
    int data_size = 0;
    if (*f_pos == 0) {
        data_size = strlen(gpio_driver_buffer);
        if (copy_to_user(buf, gpio_driver_buffer, data_size) != 0) {
            return -EFAULT;
        }
        else {
            (*f_pos) += data_size;
            return data_size;
        }
    }
    else {
        return 0;
    }
}

static ssize_t gpio_driver_write(struct file *filp, const char *buf, size_t len, loff_t *f_pos) {
    memset(gpio_driver_buffer, 0, BUF_LEN);
    if (copy_from_user(gpio_driver_buffer, buf, len) != 0) {
        return -EFAULT;
    }
    else {
        return len;
    }
}
