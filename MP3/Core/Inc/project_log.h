#ifndef INC_PROJECT_LOG_H_
#define INC_PROJECT_LOG_H_

#include <logUART/timber.h>

// The current logUART API exposes the same levels as i/w/e/s.
// Keep the project call sites source-compatible without changing classLog's ABI.
#define info i
#define warning w
#define error e
#define successful s

#endif /* INC_PROJECT_LOG_H_ */
