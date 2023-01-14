#define ASSERT(expr) ((expr)?(void)0:printk("Assert failed: '%s' in File %s Line %W\n",#expr,__FILE__,__LINE__))
#define ASSERT_ARG(expr, argfmt, arg) ((expr)?(void)0:printk("Assert failed: '%s'(" argfmt ") in File %s Line %W\n",#expr,arg,__FILE__,__LINE__))

#define push_back_array(arr, count, i, type) memmove(((type*)arr+(i)+1), ((type*)arr+(i)), ((count)-(i))*sizeof(type))
#define pull_back_array(arr, count, i, type) memmove(((type*)arr+(i)), ((type*)arr+(i)+1), ((count)-(i)-1)*sizeof(type))

#define kernel_p2v(p) (KERNEL_BASE + 0x30000 + (paddr_t)(p))
#define kernel_v2p(v) ((vaddr_t)(v) - 0x30000 - KERNEL_BASE)
