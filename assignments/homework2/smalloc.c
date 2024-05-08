#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include "smalloc.h" 
#include <string.h>


smheader_ptr smlist = 0x0 ;

void *smalloc(size_t s) {
    // 페이지 사이즈 얻기
    size_t page_size = getpagesize();

    // 페이지 사이즈보다 요청하는 크기가 크면 요청 크기로 블록 크기 설정
    size_t block_size = (s > page_size) ? s : page_size;

    // 새로운 메모리 블록 할당
    void *ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) return NULL;

    // 할당된 메모리 블록의 메타 정보 설정
    smheader_ptr header = (smheader_ptr)ptr;
    header->size = block_size - sizeof(smheader);
    header->next = NULL;
    header->used = 1; // 메모리가 사용 중임을 나타내는 플래그 설정

    // 전역 변수에 첫 번째 메모리 블록의 포인터 설정
    if (smlist == NULL) {
        smlist = header;
    } else {
        //새로운 메모리 블록을 리스트의 맨 끝에 추가
        smheader_ptr curr;
        for (curr = smlist; curr->next != NULL; curr = curr->next);
        curr->next = header;
    }

    // 할당된 메모리 블록의 데이터 영역의 포인터 반환
    return (void *)(header + 1);
}


void *smalloc_mode(size_t s, smmode m) {
    // 페이지 사이즈 얻기
    size_t page_size = getpagesize();

    // 페이지 사이즈보다 요청하는 크기가 크면 요청 크기로 블록 크기 설정
    size_t block_size = (s > page_size) ? s : page_size;

    // 적합한 메모리 블록을 찾기 위한 변수 초기화
    smheader_ptr header = NULL;
    size_t min_size = SIZE_MAX;
    size_t max_size = 0;

    // 할당 모드에 따라 메모리 블록 선택
    for (smheader_ptr current = smlist; current != NULL; current = current->next) {
        if (!current->used && current->size <= block_size) {
            switch (m) {
                case bestfit:
                    // bestfit 모드에서는 가장 작은 빈 메모리 블록 선택
                    if (current->size < min_size) {
                        header = current;
						header->used = 1;
                        min_size = current->size;
                    }
                    break;
                case worstfit:
                    // worstfit 모드에서는 가장 큰 빈 메모리 블록 선택
                    if (current->size > max_size) {
                        header = current;
						header->used = 1;
                        max_size = current->size;
                    }
                    break;
                case firstfit:
                    // firstfit 모드에서는 첫 번째로 발견된 적합한 빈 메모리 블록 선택
                    header = current;
					header->used = 1;
                    break;
            }
        }
    }

    // 적합한 메모리 블록이 없는 경우 새로운 블록을 할당
    if (header == NULL) {
		printf("assign new node\n");
        void *ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (ptr == MAP_FAILED) return NULL;

        header = (smheader_ptr)ptr;
        header->size = block_size - sizeof(smheader);
        header->next = NULL;
        header->used = 1; // 메모리가 사용 중임을 나타내는 플래그 설정

       // 전역 변수에 첫 번째 메모리 블록의 포인터 설정
    	if (smlist == NULL) {
      	  smlist = header;
 		} else {
        	//새로운 메모리 블록을 리스트의 맨 끝에 추가
        	smheader_ptr curr;
        	for (curr = smlist; curr->next != NULL; curr = curr->next);
        	curr->next = header;
    	}
    }

    // 할당된 메모리 블록의 데이터 영역의 포인터 반환
    return (void *)(header + 1);
}

void sfree (void * p) 
{
	// TODO 
	if (p == NULL) return;

    // 사용자가 전달한 포인터를 헤더로 변환
    smheader_ptr header = (smheader_ptr)p - 1;

    // 메모리 블록을 사용 중이 아니라면 이미 해제되었으므로 반환
    if (!header->used) return;

    // 메모리 블록을 해제하고 사용하지 않음으로 표시
    header->used = 0;

    // 연속된 비어있는 메모리 블록을 합침
    smcoalesce();
	return;
}

void *srealloc(void *p, size_t s) {
    // 기존 메모리 블록이 없다면 smalloc으로 새로운 메모리 블록 할당
    if (p == NULL) return smalloc(s);

    // 기존 메모리 블록의 헤더 얻기
    smheader_ptr header = (smheader_ptr)p - 1;

    // 새로운 크기에 맞게 새로운 메모리 블록 할당
    void *new_ptr = smalloc(s);
    if (new_ptr == NULL) return NULL;

    // 기존 메모리 블록의 크기보다 작은 경우, 새로운 크기에 맞게 데이터를 복사
    size_t copy_size = (header->size < s) ? header->size : s;
    memcpy(new_ptr, p, copy_size);

    // 기존 메모리 블록 해제
    sfree(p);

    return new_ptr;
}


void smcoalesce ()
{
	//TODO
}

void smdump () 
{
	smheader_ptr itr ;

	printf("==================== used memory slots ====================\n") ;
	int i = 0 ;
	for (itr = smlist ; itr != 0x0 ; itr = itr->next) {
		if (itr->used == 0)
			continue ;

		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(smheader), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(smheader) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++)  {
			printf("%02x ", s[j]) ;
		}
		printf("\n") ;
		i++ ;
	}
	printf("\n") ;

	printf("==================== unused memory slots ====================\n") ;
	i = 0 ;
	for (itr = smlist ; itr != 0x0 ; itr = itr->next, i++) {
		if (itr->used == 1)
			continue ;

		printf("%3d:%p:%8d:", i, ((void *) itr) + sizeof(smheader), (int) itr->size) ;

		int j ;
		char * s = ((char *) itr) + sizeof(smheader) ;
		for (j = 0 ; j < (itr->size >= 8 ? 8 : itr->size) ; j++) {
			printf("%02x ", s[j]) ;
		}
		printf("\n") ;
		i++ ;
	}
	printf("\n") ;
}
