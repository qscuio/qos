#include "lib.h"
#include "vlog.h"

VLOG_DEFINE_THIS_MODULE(app_vlog);

int c_vlog_file(int a, int b)
{
#define VLOG_FILE "./vlog.log"
	 vlog_set_log_file(VLOG_FILE);

     VLOG_DBG("%s: a + b = %d\n", __func__, a+b);
     VLOG_INFO("%s: a + b = %d\n", __func__, a+b);
     VLOG_EMER("%s: a + b = %d\n", __func__, a+b);
     VLOG_WARN("%s: a + b = %d\n", __func__, a+b);
     VLOG_ERR("%s: a + b = %d\n", __func__, a+b);
     VLOG_FATAL("%s: a + b = %d\n", __func__, a+b);
	 return 0;
}

int main(int argc, char **argv)
{
	c_vlog_file(10, 10);

	return 0;
}
