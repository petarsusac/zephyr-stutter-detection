#ifndef _STORAGE_H_
#define _STORAGE_H_

#define STORAGE_MAX_FILENAME_LEN (64U)
#define STORAGE_MAX_LINE_LEN (512U)

int storage_init(void);
int storage_write_line(const char *p_line, const char *p_filename);

#endif /* _STORAGE_H_ */