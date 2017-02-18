#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
 
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/kobject.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yury Adamov");
MODULE_DESCRIPTION("GPIO events counter to connect to geiger counter");
MODULE_VERSION("0.1");

static unsigned int gpioPIN = 17;
static unsigned int counter=0;
module_param(gpioPIN, uint, S_IRUGO);       ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(gpioPIN, " GPIO PIN number (default=17)");     ///< parameter description

static ssize_t count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
    struct timespec ts;
    getnstimeofday(&ts);
    return sprintf(buf,"%d %lu.%lu",counter,ts.tv_sec,ts.tv_nsec);
}

static struct kobj_attribute count_attr = __ATTR(counter, 0444, count_show, NULL);


static struct attribute *count_attrs[] = {
  &count_attr.attr,
  NULL
};

static struct attribute_group attr_group = {
  .name = "geiger",
  .attrs = count_attrs
};

static struct kobject *geiger_counter_kobj;
static int irqNum;

irq_handler_t geiger_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
  counter++;
  return (irq_handler_t) IRQ_HANDLED;
}

int __init geiger_counter_init(void){
  int res=0;

  printk(KERN_INFO "Initializing Geiger counter module...\n");
  geiger_counter_kobj = kobject_create_and_add("geiger_counter", NULL); 
  if(!geiger_counter_kobj){
      printk(KERN_ALERT "Geiger counter: failed to register in sysfs\n");
      return -ENOMEM;
  }
  
  res = sysfs_create_group(geiger_counter_kobj, &attr_group);
  if(res){
      printk(KERN_ALERT "Geiger counter: failed to create sysfs group\n");
      kobject_put(geiger_counter_kobj);
      return res;
  }
  counter=0;
  res = gpio_request(gpioPIN, "sysfs");
  if(res){
      printk("Geiger: GPIO request failure\n");
      kobject_put(geiger_counter_kobj);
      return res;
  }
  gpio_direction_input(gpioPIN);
  gpio_set_debounce(gpioPIN,1);
  
  irqNum=gpio_to_irq(gpioPIN);
  res = request_irq(irqNum, (irq_handler_t) geiger_irq_handler, IRQF_TRIGGER_RISING,"Geiger handler",NULL);
  return res;

}


void __exit geiger_counter_exit(void){
  printk(KERN_INFO "Geiger counter: exiting\n");
  kobject_put(geiger_counter_kobj);
  gpio_set_value(gpioPIN,0);
  free_irq(irqNum,NULL);
  gpio_free(gpioPIN);
}

module_init(geiger_counter_init);
module_exit(geiger_counter_exit);
