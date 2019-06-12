#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#define __NR_get_pagetable_layout 378
#define __NR_expose_page_table 356

struct pagetable_layout_info{
    uint32_t pgdir_shift;
    uint32_t pmd_shift;
    uint32_t page_shift;
};


int main(int argc, char *argv[]){
	unsigned long va;
	pid_t pid;
	int err = 0;
	unsigned long *fake_pgd_addr;
	unsigned long *fake_pmd_addr;
	unsigned long page_size;
	unsigned long pgd_size;
	unsigned long pmd_space_size;
	
	unsigned long pgd_index;
	unsigned long pmd_index;
	unsigned long offset;

	unsigned long *pmd_base;
	unsigned long page_frame;
	unsigned long physical_address;

	struct pagetable_layout_info ptb_info;


	//page_table_layout_info

	pid=atoi(argv[1]);
	va=strtoul(argv[2],NULL,16);

	printf("*******************************************************\n");
	//printf("virtual address = 0x%08lx\n", va);
	err=syscall(__NR_get_pagetable_layout, &ptb_info, sizeof(struct pagetable_layout_info));

	
	printf("pgdir_shift:%d \n",ptb_info.pgdir_shift);
	printf("pmd_shift:%d \n",ptb_info.pmd_shift);
	printf("page_shift:%d \n",ptb_info.page_shift);


	//expose_page_table
	
	page_size =  1<<(ptb_info.page_shift);
	pgd_size = (1<<(32-ptb_info.pgdir_shift))*sizeof(unsigned long);
	pmd_space_size = pgd_size * (1 << 9);

	//printf("pagesize = %ld\n", page_size);
	//printf("pgdsize = %ld\n", pgd_size);
	

	fake_pmd_addr = mmap(NULL, pmd_space_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	printf("fake_pmd_addr %lx\n", fake_pmd_addr);
	fake_pgd_addr = malloc(pgd_size);
	printf("fake_pgd_addr %lx\n", fake_pgd_addr);


	//printf("fake_pmd_addr = %p\n", fake_pmd_addr);
	//printf("fake_pgd_addr = %p\n", fake_pgd_addr);

	err = syscall(__NR_expose_page_table, pid, fake_pgd_addr, fake_pmd_addr, va, va+1);
	
	if(err != 0){
		//return -1;
	}

	//calculate the physical address;

	pgd_index = (va >> 21) & 0x7FF;
	pmd_index = (va >> ptb_info.page_shift) & 0x1FF;
	offset = va & 0xFFF;

	pmd_base = (unsigned long*)((unsigned long*)fake_pgd_addr)[pgd_index];
	//printf("pmd_base = 0x%08lx\n", pmd_base);
	page_frame = pmd_base[pmd_index] & 0xFFFFF000;
	physical_address = page_frame + offset;

	printf("VATranslate\n");
	printf("virtual address = 0x%08lx\n", va);
	printf("pgd_index = 0x%03lx\n", pgd_index);
	printf("pmd_index = 0x%03lx\n", pmd_index);

	printf("fake_pgd[0x%lx] = 0x%08lx\n", pgd_index, (unsigned long)pmd_base);
	printf("fake_pmd[0x%lx] = 0x%08lx\n", pmd_index, page_frame);
	printf("physical address = 0x%08lx\n", physical_address);
	printf("*******************************************************\n");
	free(fake_pgd_addr);
	munmap(fake_pmd_addr, pmd_space_size);
	return 0;
}
