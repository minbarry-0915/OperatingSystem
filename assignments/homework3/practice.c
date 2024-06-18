#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


int cnt = 0 ;
void* thread1 (void *) {
    while (1) {
        pthread_mutex_lock(&lock);
        if(cnt % 2 == 0){
            cnt++ ; 
            printf("thread1:%d\n", cnt) ; 
        }
        pthread_mutex_unlock(&lock);
    } 
}
void* thread2 (void *) {
    while (1) {
        pthread_mutex_lock(&lock);
        if(cnt % 2 != 0){
            cnt++ ; 
            printf("thread2:%d\n", cnt) ;    
        }
        pthread_mutex_unlock(&lock);
    } 
}
int main () {
    pthread_t t1, t2 ;
    
    pthread_create(&t1, NULL, thread1, NULL) ;  pthread_create(&t2, NULL, thread2, NULL) ;
    pthread_join(t1, NULL) ; pthread_join(t2, NULL) ;
}