#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <linux/types.h>

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern inline char * strcpy(char * dest,const char *src)
{
	char *ret = dest;
	while ((*dest++ = *src++) != '\0')
		;
	return ret;
}

extern inline char * strncpy(char * dest,const char *src,size_t count)
{
	char *ret = dest;
	while (count && *src) {
		*dest++ = *src++;
		count--;
	}
	while (count--)
		*dest++ = '\0';
	return ret;
}

extern inline char * strcat(char * dest,const char * src)
{
	char *ret = dest;
	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;
	return ret;
}

extern inline char * strncat(char * dest,const char * src,size_t count)
{
	char *ret = dest;
	while (*dest)
		dest++;
	while (count && *src) {
		*dest++ = *src++;
		count--;
	}
	*dest = '\0';
	return ret;
}

extern inline int strcmp(const char * cs,const char * ct)
{
	while (*cs && (*cs == *ct)) {
		cs++;
		ct++;
	}
	return (unsigned char)*cs - (unsigned char)*ct;
}

extern inline int strncmp(const char * cs,const char * ct,size_t count)
{
	while (count--) {
		unsigned char a = (unsigned char)*cs++;
		unsigned char b = (unsigned char)*ct++;
		if (a != b)
			return a - b;
		if (!a)
			break;
	}
	return 0;
}

extern inline char * strchr(const char * s,char c)
{
	while (*s) {
		if (*s == c)
			return (char *)s;
		s++;
	}
	if (c == '\0')
		return (char *)s;
	return NULL;
}

extern inline char * strrchr(const char * s,char c)
{
	const char *last = NULL;
	while (*s) {
		if (*s == c)
			last = s;
		s++;
	}
	if (c == '\0')
		return (char *)s;
	return (char *)last;
}

extern inline size_t strspn(const char * cs, const char * ct)
{
	size_t n = 0;
	while (*cs) {
		if (!strchr(ct, *cs))
			break;
		n++;
		cs++;
	}
	return n;
}

extern inline size_t strcspn(const char * cs, const char * ct)
{
	size_t n = 0;
	while (*cs) {
		if (strchr(ct, *cs))
			break;
		n++;
		cs++;
	}
	return n;
}

extern inline char * strpbrk(const char * cs,const char * ct)
{
	while (*cs) {
		if (strchr(ct, *cs))
			return (char *)cs;
		cs++;
	}
	return NULL;
}

extern inline char * strstr(const char * cs,const char * ct)
{
	size_t needle_len = 0;
	const char *t;

	if (!*ct)
		return (char *)cs;
	for (t = ct; *t; t++)
		needle_len++;
	while (*cs) {
		if (*cs == *ct && !strncmp(cs, ct, needle_len))
			return (char *)cs;
		cs++;
	}
	return NULL;
}

extern inline size_t strlen(const char * s)
{
	size_t n = 0;
	while (*s++)
		n++;
	return n;
}

extern char * ___strtok;

extern inline char * strtok(char * s,const char * ct)
{
	char *start;
	char *p;

	if (s)
		___strtok = s;
	if (!___strtok)
		return NULL;

	p = ___strtok;
	while (*p && strchr(ct, *p))
		p++;
	if (!*p) {
		___strtok = NULL;
		return NULL;
	}

	start = p;
	while (*p && !strchr(ct, *p))
		p++;
	if (*p) {
		*p++ = '\0';
		___strtok = p;
	} else {
		___strtok = NULL;
	}
	return start;
}

extern inline void * memcpy(void * to, const void * from, size_t n)
{
	char *d = (char *)to;
	const char *s = (const char *)from;
	while (n--)
		*d++ = *s++;
	return to;
}

extern inline void * memmove(void * dest,const void * src, size_t n)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;
	if (d < s) {
		while (n--)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}
	return dest;
}

extern inline int memcmp(const void * cs,const void * ct,size_t count)
{
	const unsigned char *a = (const unsigned char *)cs;
	const unsigned char *b = (const unsigned char *)ct;
	while (count--) {
		if (*a != *b)
			return *a - *b;
		a++;
		b++;
	}
	return 0;
}

extern inline void * memchr(const void * cs,char c,size_t count)
{
	const unsigned char *p = (const unsigned char *)cs;
	while (count--) {
		if (*p == (unsigned char)c)
			return (void *)p;
		p++;
	}
	return NULL;
}

extern inline void * memset(void * s,char c,size_t count)
{
	unsigned char *p = (unsigned char *)s;
	while (count--)
		*p++ = (unsigned char)c;
	return s;
}

	#endif
