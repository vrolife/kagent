#ifndef __kagent_common_h__
#define __kagent_common_h__

#define USED __attribute__((__used__))
#define PACKED __attribute__((packed))
#define SECTION(name) __attribute__((__section__(name)))

#define TEXT_INIT SECTION(".init.text")
#define TEXT_EXIT SECTION(".exit.text")
#define DATA_INIT SECTION(".init.data")
#define DATA_EXIT SECTION(".exit.data")

#define ALIGN_AS(n) __attribute__((__aligned__(n)))

#ifndef __MIN
#define __MIN(n, m) (n < m ? n : m)
#endif

#ifndef __MAX
#define __MAX(n, m) (n > m ? n : m)
#endif

#endif
