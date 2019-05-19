
CC = gcc

#CFLAGS = -Wall			 	\
#	-DG_DISABLE_DEPRECATED 	 	\
#	-DGDK_DISABLE_DEPRECATED 	\
#	-DGDK_PIXBUF_DISABLE_DEPRECATED \
#	-DGTK_DISABLE_DEPRECATED

CFLAGS = 
	
batteri: batteri01.c 
	$(CC) batteri01.c -o batteri $(CFLAGS) `pkg-config gtk+-2.0 --cflags --libs`	

clean: 
	rm -f *.o
