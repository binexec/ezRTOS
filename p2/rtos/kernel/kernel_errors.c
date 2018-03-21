#include "kernel_errors.h"

char* error_tostring(ERROR_CODE err)
{
	switch(err)
	{
		case NO_ERR:
		return "NO_ERR";
		break;
		
		case INVALID_ARG_ERR:
		return "INVALID_ARG_ERR";
		break;
		
		case INVALID_REQUEST_ERR:
		return "INVALID_REQUEST_ERR";
		break;
		
		case KERNEL_INACTIVE_ERR:
		return "KERNEL_INACTIVE_ERR";
		break;
		
		case UNPROCESSABLE_TASK_STATE_ERR:
		return "UNPROCESSABLE_TASK_STATE_ERR";
		break;
		
		case MAX_OBJECT_ERR:
		return "MAX_OBJECT_ERR";
		break;
		
		case OBJECT_NOT_FOUND_ERR:
		return "OBJECT_NOT_FOUND_ERR";
		break;
		
		case MALLOC_FAILED_ERR:
		return "MALLOC_FAILED_ERR";
		break;
		
		default:
		return "UNKNOWN_ERR";
	}
}



void Kernel_Error_Handler(ERROR_CODE err)
{
	
}
