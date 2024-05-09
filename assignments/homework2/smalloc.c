#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "smalloc.h" 
#include <sys/mman.h>	// mmap() 사용을 위함

smheader_ptr smlist = 0x0 ;

void * smalloc(size_t s) 
{   
    size_t pagesize = getpagesize();
    size_t blocksize = 0;
    size_t structsize = sizeof(smheader);
    if ( (s + structsize) > pagesize) {
        while (blocksize < s+structsize) {
            blocksize += pagesize ;
        }
    } else {
        blocksize = pagesize;
    }

    // smlist에 할당된 메모리 블럭이 없는 경우
    if (smlist == NULL)
    {
        // blocksize 만큼 메모리 공간 할당
        void * ptr = mmap(smlist, blocksize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

        // mmap error 인 경우 NULL 리턴
        if (ptr == MAP_FAILED) return NULL;

        // 생성된 메모리 포인터 설정
        smheader_ptr header = (smheader_ptr)ptr;

        // next 노드의 주소값
        void * nextpoint = ptr + structsize + blocksize;
        header->used = 1;
        header->size = s;
        header->next = (smheader_ptr) nextpoint;

        // smlist 에 header 정보 저장
        smlist = header;

		size_t remaining_space = blocksize - structsize - s;
        if (remaining_space > structsize) {
            // 다음 블럭 초기화
            smheader_ptr next_block = (smheader_ptr)((void *)ptr + structsize + s);
            next_block->used = 0;
            next_block->size = remaining_space - structsize;
            next_block->next = NULL;

            header->next = next_block;
        } 
		return (void *)((void *)header + structsize);
    }
    
    // smlist가 null이 아닌 경우 (할당된 메모리에 활용 가능한 공간이 있는 경우)
    smheader_ptr current;
    for (current = smlist; current != NULL; current = current->next) 
    {
        if (current->size > s + structsize && current->used == 0) {
			smheader_ptr nextnode = current->next;
			size_t nodesize = current->size;
			current->next = NULL;
			current->size = s;
			current->used = 1;

			// nextnode의 data region에 공간이 남는 경우 새로운 노드 추가
			size_t remainingSpace = nodesize - structsize - s;
			if (remainingSpace > structsize) {
				// 다음 블럭 초기화
				smheader_ptr nextblock = (smheader_ptr)((void *)current + structsize + s);
				nextblock->used = 0;
				nextblock->size = remainingSpace;
				nextblock->next = nextnode;

				current->next = nextblock;
				return (void *)((void *)current + structsize);
			}
			current->next = nextnode;
			return (void *)((void *)current + structsize);

        }
    }

	// smlist가 null이 아닌 경우 (할당된 메모리에 활용 가능한 공간이 없는 경우)
	// blocksize 만큼 메모리 공간 할당
	void *ptr = mmap(smlist, blocksize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	// mmap error 인 경우 NULL 리턴
	if (ptr == MAP_FAILED) return NULL;

	// 생성된 메모리 포인터 설정
	smheader_ptr header = (smheader_ptr)ptr;

	header->used = 1;
	header->size = s;
	header->next = NULL;

	// 리스트의 끝에 새로운 블록 추가
	for (current = smlist; current->next != NULL; current = current->next);
	current->next = header;

	size_t remainingSpace = blocksize - structsize - s;
	// data region에 공간이 남는 경우 새로운 노드 추가
	if (remainingSpace > structsize) {
		// 다음 블럭 초기화
		smheader_ptr next_block = (smheader_ptr)((void *)ptr + structsize + s);
		next_block->used = 0;
		next_block->size = remainingSpace - structsize;
		next_block->next = NULL;

		header->next = next_block;
	}

	return (void *)((void *)header + structsize);

}

void * smalloc_mode (size_t s, smmode m)
{
	size_t pagesize = getpagesize();
    size_t structsize = sizeof(smheader);

	// 적합한 메모리 블록을 찾기 위한 변수 초기화
    smheader_ptr header = NULL;
    size_t minsize = SIZE_MAX;
    size_t maxsize = 0;

	// 할당 모드에 따라 메모리 블록 선택
    smheader_ptr current;
    for (current = smlist; current != NULL; current = current->next) 
    {
		// unused면서 data의 size가 s + structsize (또는 pagesize) 보다 큰 경우
        if ( !current->used && current->size >= (s + structsize)) {
            switch (m) {
                case bestfit:
                    // bestfit 모드에서는 가장 작은 빈 메모리 블록 선택
                    if (current->size < minsize) {
                        minsize = current->size;
						header = current;
                    }
                    break;
                case worstfit:
                    // worstfit 모드에서는 가장 큰 빈 메모리 블록 선택
                    if (current->size > maxsize) {
						maxsize = current->size;
						header = current;
                    }
                    break;
                case firstfit:
					header = current;
					smheader_ptr nextnode = header->next;
					size_t nodesize = header->size;
					header->next = NULL;
					header->size = s;
					header->used = 1;

					// nextnode의 data region에 공간이 남는 경우 새로운 노드 추가
					size_t remainingSpace = nodesize - structsize - s;
					if (remainingSpace > structsize) {
						// 다음 블럭 초기화
						smheader_ptr nextblock = (smheader_ptr)((void *)header + structsize + s);
						nextblock->used = 0;
						nextblock->size = remainingSpace;
						nextblock->next = nextnode;

						header->next = nextblock;
						return (void *)((void *)header + structsize);
					}
					
					header->next = nextnode;
					return (void *)((void *)header + structsize);

					break;
            }
        }
    }

	// mode 유형에 해당하는 메모리 블럭을 찾은 경우
	if (header != NULL) {
		smheader_ptr nextnode = header->next;
		size_t nodesize = header->size;
		header->next = NULL;
		header->size = s;
		header->used = 1;

		// data region에 공간이 남는 경우 새로운 노드 추가
		size_t remainingSpace = nodesize - structsize - s;
		if (remainingSpace > structsize) {
			// 다음 블럭 초기화
			smheader_ptr nextblock = (smheader_ptr)((void *)header + structsize + s);
			nextblock->used = 0;
			nextblock->size = remainingSpace;
			nextblock->next = nextnode;

			header->next = nextblock;
			return (void *)((void *)header + structsize);
		}
		
		header->next = nextnode;
		return (void *)((void *)header + structsize);

	} else {
		// 적합한 메모리 블록이 없는 경우 새로운 블록을 할당
		smalloc(s);
	}
	
}

void sfree (void * p) 
{
    smheader_ptr current = smlist;
	size_t structsize = sizeof(smheader);

    // smlist를 순회하면서 p에 해당하는 메모리 블록을 찾음
    while (current != NULL) {
        if ((void *)current + structsize == p) {
            current->used = 0;
            return;
        }
        current = current->next;
    }
	// 메모리 블럭을 찾지 못한 경우 abort 발생
	abort();
}

void * srealloc (void * p, size_t s) 
{
	smheader_ptr next = NULL;
    smheader_ptr current = smlist;
	size_t structsize = sizeof(smheader);

    // smlist를 순회하면서 p에 해당하는 메모리 블록을 찾음
    for (current = smlist; current != NULL; current = current->next)  {
        if ((void *)current + structsize == p) {
			next = current->next;
			
			// size가 0이면 메모리 해제
			if ( !s ) {
				sfree(p);
				return p;
			}

			// 기존 메모리 크기보다 큰 size로 realloc 하려는 경우 현재 메모리 free 하고 smalloc_mode로 할당
			else if (current->size < s) {
				void * ptr = smalloc_mode(s, bestfit);
				sfree(p);
				return ptr;
			}

			// 기존 메모리 크기보다 작은 size로 realloc 하려는 경우 현재 메모리 free 하고 새로 할당하기
			else {
				size_t nodesize = current->size;
				current->size = s;	// 입력받은 size로 업데이트
				current->used = 1;

				// data region에 공간이 남는 경우 새로운 노드 추가
				size_t remainingSpace = nodesize - structsize - s;
				printf("remaining size: %ld",remainingSpace);
				if (remainingSpace > structsize) {
					// 다음 블럭 초기화
					smheader_ptr nextblock = (smheader_ptr)((void *)current + structsize + s);
					nextblock->used = 0;
					nextblock->size = remainingSpace;
					nextblock->next = next;

					current->next = nextblock;
				}

				return (void *)current + structsize;
			}
		}	
	}
	// 메모리 블럭을 찾지 못한 경우 abort 발생
	abort();
}

void smcoalesce ()
{
	smheader_ptr nextnode = NULL;
	smheader_ptr nnextnode = NULL;
    smheader_ptr current = smlist;
	size_t structsize = sizeof(smheader);

	// smlist를 순회하면서 비어있는 메모리 블록을 찾음
    for (current = smlist; current->next != NULL; current = current->next)  {
		
		// current가 unused이면서 current->next도 unuesd 라면
		if ( current->next != NULL )
			nextnode = current->next;
		if( !current->used && !nextnode->used ) {
			nnextnode = nextnode->next; // node 연결을 위한 nextnode->next 의 주소값 저장
			current->size = current->size + nextnode->size + structsize;
			current->next = nnextnode;
			munmap((void *)nextnode, structsize + nextnode->size);
		}
	}
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
