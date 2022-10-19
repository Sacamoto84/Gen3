#ifndef SRC_LOG_H_
#define SRC_LOG_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

extern void LOG_init(void);
extern void LOG(char * tag, char TYPE, char * text);

#ifdef __cplusplus
}
#endif

#endif /* SRC_LOG_H_ */
