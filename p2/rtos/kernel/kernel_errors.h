#ifndef KERNEL_ERRORS_H_
#define KERNEL_ERRORS_H_

#include "kernel_shared.h"


/*Definitions for potential errors the RTOS may come across*/
typedef enum ERROR_CODE
{
	NO_ERR  = 0,
	INVALID_ARG_ERR,
	INVALID_REQUEST_ERR,
	KERNEL_INACTIVE_ERR,
	UNPROCESSABLE_TASK_STATE_ERR,
	MAX_OBJECT_ERR,
	OBJECT_NOT_FOUND_ERR,
	MALLOC_FAILED_ERR,
	UNKNOWN_ERR
	
} ERROR_CODE;



char* error_tostring(ERROR_CODE);
void Kernel_Error_Handler(ERROR_CODE err);



//TODO: Print all error related to stderr instead. We're using printf/stdout for now because there seem to be a big with fprintf(stderr, ...)

#define kernel_raise_error(err_code) {																		\
	err = err_code;																							\
	printf("%s: \"%s\" raised at %s:%d\n", __FUNCTION__, error_tostring(err_code), __FILE__, __LINE__);		\
	Kernel_Error_Handler(err_code);																			\
}


#endif /* KERNEL_ERRORS_H_ */