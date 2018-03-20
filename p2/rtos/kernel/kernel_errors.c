#include "kernel_errors.h"

char* error_tostring(ERROR_CODE err)
{
	switch(err)
	{
		case NO_ERR:
		return "NO_ERR";
		
		case INVALID_ARG_ERR:
		return "INVALID_ARG_ERR";
		
		case INVALID_REQUEST_ERR:
		return "INVALID_REQUEST_ERR";
		
		case KERNEL_INACTIVE_ERR:
		return "KERNEL_INACTIVE_ERR";
		
		case UNPROCESSABLE_TASK_STATE_ERR:
		return "UNPROCESSABLE_TASK_STATE_ERR";
		
		case MAX_OBJECT_ERR:
		return "MAX_OBJECT_ERR";
		
		case OBJECT_NOT_FOUND_ERR:
		return "OBJECT_NOT_FOUND_ERR";
		
		case MALLOC_FAILED_ERR:
		return "MALLOC_FAILED_ERR";
		
		default:
		return "UNKNOWN_ERR";
	}
}



void Kernel_Error_Handler(ERROR_CODE err)
{
	
}
