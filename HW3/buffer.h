typedef struct {
	char *arr;
	int size;
	int nextIn;
	int nextOut;
	semaphore *emptySemaphore;
	semaphore *fullSemaphore;
} Buffer;

Buffer* buffer_create(int size);

char buffer_remove(Buffer *b);

void buffer_deposit(Buffer *b, char c);

void buffer_free(Buffer *b);
