/*
 * Parametri kernel modula:
 *   steppers_count  (int)       - broj steper motora
 *   steppers_step  (int arr)   - niz pinova na koje su steperi povezani (GPIO_23 bi bio 23 samo)
 *   steppers_en     (int arr)   - niz pinova za steper enable
 * 
 * Cheatsheet:
 *   lsmod                                - izlistavanje
 *   sudo insmod gpio_driver.ko           - ubacivanje
 *   sudo rmmod gpio_driver               - izbacivanje
 *   modinfo gpio_driver                  - info
 *   dmesg -wH                            - real-time log
 *   sudo insmod ... myArray=1,2          - Parametri
 *   sudo mknod /dev/gpio_driver c 239 0  - pravljenje node-a sa major brojem 239
 *   sudo chmod 666 /dev/gpio_driver      - menjanje prava nad node-om
 *   echo "Hello" > /dev/chardev          - Upis u node
 *   cat /dev/chardev                     - Citanje iz node-a
*/

/* Libraries */
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
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <asm/io.h>

/* Module info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marko Đorđević, Radomir Zlatković, Aleksa Heler");
MODULE_DESCRIPTION("Midi keyboard to stepper motor interface using kernel module");

/* Najveci broj podrzanih stepera (potrebno za nizove) */
#define MAX_STEPPERS 4

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
//000 = GPIO Pin 'x' is an input
//001 = GPIO Pin 'x' is an output
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

/* Indeks je broj stepera */
static int steppers_count = 1;                   /* Broj stepper motora */
static int steppers_step[MAX_STEPPERS];         /* Step pinovi stepera*/
static int steppers_en[MAX_STEPPERS];           /* Enable pinovi stepera */
static int steppers_power[MAX_STEPPERS];        /* Trenutna vrednost napona na pinu */
static int steppers_history[MAX_STEPPERS];      /* Zadnje svirana nota stepera */
static int steppers_ticks[MAX_STEPPERS];        /* Merenje vremena da nota ne svira beskonacno */
static int steppers_max_ticks[MAX_STEPPERS];    /* -||- ovo je max vrednost za ticks */

/* Otkrivanje parametara da korisnik moze postaviti */
module_param(steppers_count, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(steppers_count, "Broj povezanih steper motora");
module_param_array(steppers_step, int, &steppers_count, 0000);
MODULE_PARM_DESC(steppers_step, "Array of stepper step pins");
module_param_array(steppers_en, int, &steppers_count, 0000);
MODULE_PARM_DESC(steppers_en, "Array of stepper enable pins");

/* One timer per stepper */
static struct hrtimer pwm_timers[MAX_STEPPERS];   /* Timers array */
static ktime_t kt[MAX_STEPPERS];

int gpio_driver_major;              /* Major number. */
#define BUF_LEN 80                  /* Buffer to store data. */
char* gpio_driver_buffer;
void* virt_gpio_base;               /* Virtual address where the physical GPIO address is mapped */

/*
 * GetGPFSELReg function
 *  Parameters:
 *   pin    - number of GPIO pin;
 *
 *   return - GPFSELn offset from GPIO base address, for containing desired pin control
 *  Operation:
 *   Based on the passed GPIO pin number, finds the corresponding GPFSELn reg and
 *   returns its offset from GPIO base address.
 */
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

/*
 * GetGPIOPinOffset function
 *  Parameters:
 *   pin    - number of GPIO pin;
 *
 *   return - offset of the pin control bit, position in control registers
 *  Operation:
 *   Based on the passed GPIO pin number, finds the position of its control bit
 *   in corresponding control registers.
 */
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

/*
 * SetInternalPullUpDown function
 *  Parameters:
 *   pin    - number of GPIO pin;
 *   pull   - set internal pull up/down/none if PULL_UP/PULL_DOWN/PULL_NONE selected
 *  Operation:
 *   Sets to use internal pull-up or pull-down resistor, or not to use it if pull-none
 *   selected for desired GPIO pin.
 */
void SetInternalPullUpDown(char pin, PUD pull)
{
    unsigned int gppud_offset;
    unsigned int gppudclk_offset;
    unsigned int tmp;
    unsigned int mask;

    /* Get the offset of GPIO Pull-up/down Register (GPPUD) from GPIO base address. */
    gppud_offset = GPPUD_OFFSET;
    /* Get the offset of GPIO Pull-up/down Clock Register (GPPUDCLK) from GPIO base address. */
    gppudclk_offset = (pin < 32) ? GPPUDCLK0_OFFSET : GPPUDCLK1_OFFSET;
    /* Get pin offset in register . */
    pin = (pin < 32) ? pin : pin - 32;
    /* Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
       to remove the current Pull-up/down). */
    iowrite32(pull, virt_gpio_base + gppud_offset);
    /* Wait 150 cycles � this provides the required set-up time for the control signal */
    /* Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
       modify � NOTE only the pads which receive a clock will be modified, all others will
       retain their previous state. */
    tmp = ioread32(virt_gpio_base + gppudclk_offset);
    mask = 0x1 << pin;
    tmp |= mask;
    iowrite32(tmp, virt_gpio_base + gppudclk_offset);
    /* Wait 150 cycles � this provides the required hold time for the control signal */
    /* Write to GPPUD to remove the control signal. */
    iowrite32(PULL_NONE, virt_gpio_base + gppud_offset);
    /* Write to GPPUDCLK0/1 to remove the clock. */
    tmp = ioread32(virt_gpio_base + gppudclk_offset);
    mask = 0x1 << pin;
    tmp &= (~mask);
    iowrite32(tmp, virt_gpio_base + gppudclk_offset);
}

/*
 * SetGpioPinDirection function
 *  Parameters:
 *   pin       - number of GPIO pin;
 *   direction - GPIO_DIRECTION_IN or GPIO_DIRECTION_OUT
 *  Operation:
 *   Sets the desired GPIO pin to be used as input or output based on the direcation value.
 */
void SetGpioPinDirection(char pin, DIRECTION direction)
{
    unsigned int GPFSELReg_offset;
    unsigned int tmp;
    unsigned int mask;

    /* Get base address of function selection register. */
    GPFSELReg_offset = GetGPFSELReg(pin);
    /* Calculate gpio pin offset. */
    pin = GetGPIOPinOffset(pin);
    /* Set gpio pin direction. */
    tmp = ioread32(virt_gpio_base + GPFSELReg_offset);
    if(direction)
    { //set as output: set 1
      mask = 0x1 << (pin*3);
      tmp |= mask;
    }
    else
    { //set as input: set 0
      mask = ~(0x1 << (pin*3));
      tmp &= mask;
    }
    iowrite32(tmp, virt_gpio_base + GPFSELReg_offset);
}

/*
 * SetGpioPin function
 *  Parameters:
 *   pin       - number of GPIO pin;
 *  Operation:
 *   Sets the desired GPIO pin to HIGH level. The pin should previously be defined as output.
 */
void SetGpioPin(char pin)
{
    unsigned int GPSETreg_offset;
    unsigned int tmp;
    
    /* Get base address of gpio set register. */
    GPSETreg_offset = (pin < 32) ? GPSET0_OFFSET : GPSET1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    /* Set gpio. */
    tmp = 0x1 << pin;
    iowrite32(tmp, virt_gpio_base + GPSETreg_offset);
}

/*
 * ClearGpioPin function
 *  Parameters:
 *   pin       - number of GPIO pin;
 *  Operation:
 *   Sets the desired GPIO pin to LOW level. The pin should previously be defined as output.
 */
void ClearGpioPin(char pin)
{
    unsigned int GPCLRreg_offset;
    unsigned int tmp;
    
    /* Get base address of gpio clear register. */
    GPCLRreg_offset = (pin < 32) ? GPCLR0_OFFSET : GPCLR1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;

    /* Clear gpio. */
    tmp = 0x1 << pin;
    iowrite32(tmp, virt_gpio_base + GPCLRreg_offset);
}

/*
 * GetGpioPinValue function
 *  Parameters:
 *   pin       - number of GPIO pin;
 *
 *   return    - the level read from desired GPIO pin
 *  Operation:
 *   Reads the level from the desired GPIO pin and returns the read value.
 */
char GetGpioPinValue(char pin)
{
    unsigned int GPLEVreg_offset;
    unsigned int tmp;
    unsigned int mask;

    /* Get base address of gpio level register. */
    GPLEVreg_offset = (pin < 32) ?  GPLEV0_OFFSET : GPLEV1_OFFSET;
    pin = (pin < 32) ? pin : pin - 32;
    /* Read gpio pin level. */
    tmp = ioread32(virt_gpio_base + GPLEVreg_offset);
    mask = 0x1 << pin;
    tmp &= mask;
    return (tmp >> pin);
}

/* timer callback function called each time the timer expires */
static enum hrtimer_restart pwm_timer_callback(struct hrtimer *param) {

    /* TODO: Make this function change only the stepper it is assigned to from stepper array */
    
    /* Switch voltage on stepper pin */
    steppers_power[0] ^= 0x1;

    if (steppers_power[0])
        SetGpioPin(steppers_step[0]);
    else
        ClearGpioPin(steppers_step[0]);

    /* Da ne bi radio beskonacno samo se prekine nakon odredjenog broja periode */
    if(++steppers_ticks[0] == steppers_max_ticks[0]){
        SetGpioPin(steppers_en[0]); /* Disable stepper */
        return HRTIMER_NORESTART;
    } 

    hrtimer_forward(&pwm_timers[0], ktime_get(), kt[0]);
    return HRTIMER_RESTART;
}

/*
 * Initialization:
 *  1. Register device driver
 *  2. Allocate buffer
 *  3. Initialize buffer
 *  4. Map GPIO Physical address space to virtual address
 *  5. Initialize GPIO pins
 *  6. Init and start the high resoultion timer
 */
int gpio_driver_init(void){
    int result = -1;
    int i;

    printk(KERN_INFO "Inserting gpio_driver module\n");

    /* Registering device. */
    result = register_chrdev(0, "gpio_driver", &gpio_driver_fops);
    if (result < 0) {
        printk(KERN_INFO "gpio_driver: cannot obtain major number %d\n", gpio_driver_major);
        return result;
    }

    gpio_driver_major = result;
    printk(KERN_INFO "gpio_driver major number is %d\n", gpio_driver_major);

    /* Allocating memory for the buffer. */
    gpio_driver_buffer = kmalloc(BUF_LEN, GFP_KERNEL);
    if (!gpio_driver_buffer) {
        result = -ENOMEM;
        goto fail_no_mem;
    }

    /* Initialize data buffer. */
    memset(gpio_driver_buffer, 0, BUF_LEN);

    /* map the GPIO register space from PHYSICAL address space to virtual address space */
    virt_gpio_base = ioremap(GPIO_BASE, GPIO_ADDR_SPACE_LEN);
    if(!virt_gpio_base)
    {
        result = -ENOMEM;
        goto fail_no_virt_mem;
    }

    /* Initialize all stepper GPIO pins from array of steppers */
    printk(KERN_INFO "Steppers:\n");
    for (i = 0; i < steppers_count; i++)
    {
        printk(KERN_INFO "  [%d]  step_pin: %d    en_pin: %d", i, steppers_step[i], steppers_en[i]);
        SetGpioPinDirection(steppers_step[i], GPIO_DIRECTION_OUT);
        ClearGpioPin(steppers_step[i]);
        /* Initially disable steppers */
        SetGpioPinDirection(steppers_en[i], GPIO_DIRECTION_OUT);
        SetGpioPin(steppers_en[i]);
    }

    /* initiate all necessary arrays to default values */
    for (i = 0; i < steppers_count; i++)
    {
        steppers_history[i] = 0xFF;
        steppers_ticks[i] = 0;
        steppers_max_ticks[i] = 0;
    }

    //Timer init
    printk(KERN_INFO "Timers:\n");
    for (i = 0; i < steppers_count; i++)
    {
        printk(KERN_INFO "pwm_timers[%d]", i);
        hrtimer_init(&pwm_timers[i], CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    }

    return 0;

fail_no_virt_mem:
    /* Freeing buffer gpio_driver_buffer. */
    if (gpio_driver_buffer)
        kfree(gpio_driver_buffer);
fail_no_mem:
    /* Freeing the major number. */
    unregister_chrdev(gpio_driver_major, "gpio_driver");
    return result;
}

/*
 * Cleanup:
 *  1. stop the timer
 *  2. release GPIO pins (clear all outputs, set all as inputs and pull-none to minimize the power consumption)
 *  3. Unmap GPIO Physical address space from virtual address
 *  4. Free buffer
 *  5. Unregister device driver
 */
void gpio_driver_exit(void) {
    int i;
    
    printk(KERN_INFO "Removing gpio_driver module\n");

    /* Release high resolution timer. */
    hrtimer_cancel(&pwm_timers[0]);

    /* Clear GPIO pins. */
    for(i = 0; i < steppers_count; i++){
        /* Set voltage to low */
        ClearGpioPin(steppers_step[i]);
        ClearGpioPin(steppers_en[i]);
        /* Set GPIO pins as inputs and disable pull-ups. */
        SetGpioPinDirection(steppers_step[i], GPIO_DIRECTION_IN);
        SetGpioPinDirection(steppers_en[i], GPIO_DIRECTION_IN);
    }

    /* Unmap GPIO Physical address space. */
    if (virt_gpio_base) {
        iounmap(virt_gpio_base);
    }

    /* Freeing buffer gpio_driver_buffer. */
    if (gpio_driver_buffer) {
        kfree(gpio_driver_buffer);
    }

    /* Freeing the major number. */
    unregister_chrdev(gpio_driver_major, "gpio_driver");
}

/* File open function. */
static int gpio_driver_open(struct inode *inode, struct file *filp)
{
    // TODO: Initialize driver variables

    // Success. 
    return 0;
}

/* File close function. */
static int gpio_driver_release(struct inode *inode, struct file *filp){
    int i;
    
    /* Stop the timers and disable steppers because no one is writing to node */
    hrtimer_cancel(&pwm_timers[0]);
    for(i = 0; i < steppers_count; i++){
        SetGpioPin(steppers_en[i]);
    }
    
    return 0;
}

/*
 * File read function
 *  Parameters:
 *   filp  - a type file structure;
 *   buf   - a buffer, from which the user space function (fread) will read;
 *   len - a counter with the number of bytes to transfer, which has the same
 *           value as the usual counter in the user space function (fread);
 *   f_pos - a position of where to start reading the file;
 *  Operation:
 *   The gpio_driver_read function transfers data from the driver buffer (gpio_driver_buffer)
 *   to user space with the function copy_to_user.
 */
static ssize_t gpio_driver_read(struct file *filp, char *buf, size_t len, loff_t *f_pos){
    int data_size = 0;

    /* TODO: User can read all stepper pins */
    /* Something is not working, 'cat' not returning anything */
    int i;
    for (i = 0; i < steppers_count; i++)
    {
        gpio_driver_buffer[i*2] = i;
        gpio_driver_buffer[i*2+1] = steppers_step[i];
    }
    gpio_driver_buffer[i] = '\0';
    printk(KERN_INFO "Writing %d bytes of stepper pins info to user", strlen(gpio_driver_buffer));

    if (*f_pos == 0) {
        /* Get size of valid data. */
        data_size = strlen(gpio_driver_buffer);
        /* Send data to user space. */
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

struct MIDIStruct {
    const int MIDINumber;
    const int period;
    const int ticks;
};

const struct MIDIStruct MIDITable[88] = {
//MIDINumber,  period(ms), ticks_normal
    {21,      36.36 * 1000,   7},   //    A0
    {22,      34.32 * 1000,   7},   // A0/B0
    {23,      32.40 * 1000,   8},   //    B0
    {24,      30.58 * 1000,   8},   //    C1
    {25,      28.86 * 1000,   9},   // C1/D1
    {26,      27.24 * 1000,   9},   //    D1
    {27,      25.71 * 1000,   10},   // D1/E1
    {28,      24.27 * 1000,   10},   //    E1
    {29,      22.91 * 1000,   11},   //    F1
    {30,      21.26 * 1000,   12},   // F1/G1
    {31,      20.41 * 1000,   12},   //    G1
    {32,      19.26 * 1000,   13},   // G1/A1
    {33,      18.18 * 1000,   14},   //    A1
    {34,      17.16 * 1000,   15},   // A1/B1
    {35,      16.20 * 1000,   15},   //    B1
    {36,      15.29 * 1000,   16},   //    C2
    {37,      14.29 * 1000,   17},   // C2/D2
    {38,      13.62 * 1000,   18},   //    D2
    {39,      12.86 * 1000,   19},   // D2/E2
    {40,      12.13 * 1000,   21},   //    E2
    {41,      11.45 * 1000,   22},   //    F2
    {42,      10.81 * 1000,   23},   // F2/G2
    {43,      10.20 * 1000,   25},   //    G2
    {44,      9.631 * 1000,   26},   // G2/A2
    {45,      9.091 * 1000,   27},   //    A2
    {46,      8.581 * 1000,   30},   // A2/B2
    {47,      8.099 * 1000,   31},   //    B2
    {48,      7.645 * 1000,   33},   //    C3
    {49,      7.216 * 1000,   35},   // C3/D3
    {50,      6.811 * 1000,   37},   //    D3
    {51,      6.428 * 1000,   39},   // D3/E3
    {52,      6.068 * 1000,   41},   //    E3
    {53,      5.727 * 1000,   44},   //    F3
    {54,      5.405 * 1000,   46},   // F3/G3
    {55,      5.102 * 1000,   49},   //    G3
    {56,      4.816 * 1000,   52},   // G3/A3
    {57,      4.545 * 1000,   55},   //    A3
    {58,      4.290 * 1000,   58},   // A3/B3
    {59,      4.050 * 1000,   62},   //    B3
    {60,      3.822 * 1000,   65},   //    C4
    {61,      3.608 * 1000,   69},   // C4/D4
    {62,      3.405 * 1000,   73},   //    D4
    {63,      3.214 * 1000,   78},   // D4/E4
    {64,      3.034 * 1000,   82},   //    E4
    {65,      2.863 * 1000,   87},   //    F4
    {66,      2.703 * 1000,   92},   // F4/G4
    {67,      2.551 * 1000,   98},   //    G4
    {68,      2.408 * 1000,   104},   // G4/A4
    {69,      2.273 * 1000,   110},   //    A4
    {70,      2.145 * 1000,   117},   // A4/B4
    {71,      2.025 * 1000,   123},   //    B4
    {72,      1.910 * 1000,   131},   //    C5
    {73,      1.804 * 1000,   139},   // C5/D5
    {74,      1.703 * 1000,   147},   //    D5
    {75,      1.607 * 1000,   156},   // D5/E5
    {76,      1.517 * 1000,   165},   //    E5
    {77,      1.432 * 1000,   175},   //    F5
    {78,      1.351 * 1000,   185},   // F5/G5
    {79,      1.276 * 1000,   196},   //    G5
    {80,      1.204 * 1000,   208},   // G5/A5
    {81,      1.136 * 1000,   220},   //    A5
    {82,      1.073 * 1000,   233},   // A5/B5
    {83,      1.012 * 1000,   247},   //    B5
    {84,     0.9556 * 1000,   262},  //    C6
    {85,     0.9020 * 1000,   277},  // C6/D6
    {86,     0.8513 * 1000,   294},  //    D6
    {87,     0.8034 * 1000,   311},  // D6/E6
    {88,     0.7584 * 1000,   330},  //    E6
    {89,     0.7159 * 1000,   349},  //    F6
    {90,     0.6757 * 1000,   370},  // F6/G6
    {91,     0.6378 * 1000,   392},  //    G6
    {92,     0.6020 * 1000,   415},  // G6/A6
    {93,     0.5682 * 1000,   440},  //    A6
    {94,     0.5363 * 1000,   466},  // A6/B6
    {95,     0.5062 * 1000,   494},  //    B6
    {96,     0.4778 * 1000,   523},  //    C7
    {97,     0.4510 * 1000,   554},  // C7/D7
    {98,     0.4257 * 1000,   587},  //    D7
    {99,     0.4018 * 1000,   622},  // D7/E7
    {100,    0.3792 * 1000,   659}, //    E7
    {101,    0.3580 * 1000,   698}, //    F7
    {102,    0.3378 * 1000,   740}, // F7/G7
    {103,    0.3189 * 1000,   786}, //    G7
    {104,    0.3010 * 1000,   831}, // G7/A7
    {105,    0.2841 * 1000,   880}, //    A7
    {106,    0.2681 * 1000,   932}, // A7/B7
    {107,    0.2531 * 1000,   988}, //    B7
    {108,    0.2389 * 1000,   1046}  //    C8
};

/*
 * File write function
 *  Parameters:
 *   filp  - a type file structure;
 *   buf   - a buffer in which the user space function (fwrite) will write;
 *   len - a counter with the number of bytes to transfer, which has the same
 *           values as the usual counter in the user space function (fwrite);
 *   f_pos - a position of where to start writing in the file;
 *  Operation:
 *   The function copy_from_user transfers the data from user space to kernel space.
 */
static ssize_t gpio_driver_write(struct file *filp, const char *buf, size_t len, loff_t *f_pos) {
    /* Reset memory */
    memset(gpio_driver_buffer, 0, BUF_LEN);
    /* Get data from user space */
    if (copy_from_user(gpio_driver_buffer, buf, len) != 0) {
        return -EFAULT;
    }
    else {
        if(len == 2){ // Got a note

            /* Ponovo pocinje merenje vremena za max trajanje note */
            steppers_ticks[0] = 0;

            /* Prekine se prosla nota ako se razlikuje od one koju je do sada svirao */
            if(gpio_driver_buffer[1] != steppers_history[0]){
                hrtimer_cancel(&pwm_timers[0]);
                SetGpioPin(steppers_en[0]); /* Disable stepper to stop wasting current */
                steppers_history[0] = gpio_driver_buffer[1];
            }

            /* No stop signal */
            if (gpio_driver_buffer[1] != 0xFF) {
                /* Enable stepper so it will be able to play the note */
                ClearGpioPin(steppers_en[0]);
                /* Print for debug */
                printk(KERN_INFO "%d -> note %d, period = %d\n", gpio_driver_buffer[0], gpio_driver_buffer[1], MIDITable[ gpio_driver_buffer[1] - 21].period);
                /* Postavi max count na vrednost iz tabele puta 40 da duze traje */
                steppers_max_ticks[0] = MIDITable[ gpio_driver_buffer[1] - 21 ].ticks * 40;
                
                /* Set interval for high resolution timer */
                kt[0] = ktime_set(0, MIDITable[ gpio_driver_buffer[1] - 21 ].period * 500);
                /* Set callback function */
                pwm_timers[0].function = &pwm_timer_callback;
                /* Start timer */
                hrtimer_start(&pwm_timers[0], kt[0], HRTIMER_MODE_REL);
            } 
            /* Stop signal [0xFF] */
            else {
                hrtimer_cancel(&pwm_timers[0]);
                SetGpioPin(steppers_en[0]); /* Disable stepper to stop wasting current */
                steppers_history[0] = gpio_driver_buffer[1];
            }
            
            return len;
        }
        else{
            printk(KERN_INFO "[Error] Received %d bytes, expecting 2", len);
        }
    }

    return len;        
}
