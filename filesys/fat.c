#include "filesys/fat.h"
#include "devices/disk.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include <stdio.h>
#include <string.h>

/* Should be less than DISK_SECTOR_SIZE */
struct fat_boot {
	unsigned int magic;
	unsigned int sectors_per_cluster; /* Fixed to 1 */
	unsigned int total_sectors;
	unsigned int fat_start;
	unsigned int fat_sectors; /* Size of FAT in sectors. */
	unsigned int root_dir_cluster;
};

/* FAT FS */
struct fat_fs {
	struct fat_boot bs;
	unsigned int *fat;
	unsigned int fat_length;
	disk_sector_t data_start;
	cluster_t last_clst;
	struct lock write_lock;
};

static struct fat_fs *fat_fs;

void fat_boot_create (void);
void fat_fs_init (void);

void
fat_init (void) {
	fat_fs = calloc (1, sizeof (struct fat_fs));
	if (fat_fs == NULL)
		PANIC ("FAT init failed");

	// Read boot sector from the disk
	unsigned int *bounce = malloc (DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT init failed");
	disk_read (filesys_disk, FAT_BOOT_SECTOR, bounce);
	memcpy (&fat_fs->bs, bounce, sizeof (fat_fs->bs));
	free (bounce);

	// Extract FAT info
	if (fat_fs->bs.magic != FAT_MAGIC)
		fat_boot_create ();
	fat_fs_init ();
}

void
fat_open (void) {
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT load failed");

	// Load FAT directly from the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_read = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		bytes_left = fat_size_in_bytes - bytes_read;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_read (filesys_disk, fat_fs->bs.fat_start + i,
			           buffer + bytes_read);
			bytes_read += DISK_SECTOR_SIZE;
		} else {
			uint8_t *bounce = malloc (DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT load failed");
			disk_read (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			memcpy (buffer + bytes_read, bounce, bytes_left);
			bytes_read += bytes_left;
			free (bounce);
		}
	}
}

void
fat_close (void) {
	// Write FAT boot sector
	uint8_t *bounce = calloc (1, DISK_SECTOR_SIZE);
	if (bounce == NULL)
		PANIC ("FAT close failed");
	memcpy (bounce, &fat_fs->bs, sizeof (fat_fs->bs));
	disk_write (filesys_disk, FAT_BOOT_SECTOR, bounce);
	free (bounce);

	// Write FAT directly to the disk
	uint8_t *buffer = (uint8_t *) fat_fs->fat;
	off_t bytes_wrote = 0;
	off_t bytes_left = sizeof (fat_fs->fat);
	const off_t fat_size_in_bytes = fat_fs->fat_length * sizeof (cluster_t);
	for (unsigned i = 0; i < fat_fs->bs.fat_sectors; i++) {
		bytes_left = fat_size_in_bytes - bytes_wrote;
		if (bytes_left >= DISK_SECTOR_SIZE) {
			disk_write (filesys_disk, fat_fs->bs.fat_start + i,
			            buffer + bytes_wrote);
			bytes_wrote += DISK_SECTOR_SIZE;
		} else {
			bounce = calloc (1, DISK_SECTOR_SIZE);
			if (bounce == NULL)
				PANIC ("FAT close failed");
			memcpy (bounce, buffer + bytes_wrote, bytes_left);
			disk_write (filesys_disk, fat_fs->bs.fat_start + i, bounce);
			bytes_wrote += bytes_left;
			free (bounce);
		}
	}
}

void
fat_create (void) {
	// Create FAT boot
	fat_boot_create ();
	fat_fs_init ();

	// Create FAT table
	fat_fs->fat = calloc (fat_fs->fat_length, sizeof (cluster_t));
	if (fat_fs->fat == NULL)
		PANIC ("FAT creation failed");

	// Set up ROOT_DIR_CLST
	fat_put (ROOT_DIR_CLUSTER, EOChain);

	// Fill up ROOT_DIR_CLUSTER region with 0
	uint8_t *buf = calloc (1, DISK_SECTOR_SIZE);
	if (buf == NULL)
		PANIC ("FAT create failed due to OOM");
	disk_write (filesys_disk, cluster_to_sector (ROOT_DIR_CLUSTER), buf);
	free (buf);
}

void
fat_boot_create (void) {
	unsigned int fat_sectors =
	    (disk_size (filesys_disk) - 1)
	    / (DISK_SECTOR_SIZE / sizeof (cluster_t) * SECTORS_PER_CLUSTER + 1) + 1;
	fat_fs->bs = (struct fat_boot){
	    .magic = FAT_MAGIC,
	    .sectors_per_cluster = SECTORS_PER_CLUSTER,
	    .total_sectors = disk_size (filesys_disk),
	    .fat_start = 1,
	    .fat_sectors = fat_sectors,
	    .root_dir_cluster = ROOT_DIR_CLUSTER,
	};
}

/* FAT 파일 시스템을 초기화합니다. fat_fs의 fat_length 및 data_start 필드를 초기화해야 합니다.
 * fat_length는 파일 시스템의 클러스터 수를 저장하고 data_start는 파일 저장을 시작할 수 있는
 * 섹터를 저장합니다. fat_fs->bs에 저장된 일부 값을 악용할 수 있습니다.
 * 또한 이 함수에서 다른 유용한 데이터를 초기화할 수 있습니다. */
void
fat_fs_init (void) {
	/* TODO: Your code goes here. */

	fat_fs->fat_length = fat_fs->bs.fat_sectors / sizeof(cluster_t);
	// fat_fs->fat_length = 0;
	fat_fs->data_start = fat_fs->bs.fat_start;
	fat_fs->last_clst = fat_fs->bs.fat_start;

}

/*----------------------------------------------------------------------------*/
/* FAT handling                                                               */
/*----------------------------------------------------------------------------*/

/* Add a cluster to the chain.
 * If CLST is 0, start a new chain.
 * Returns 0 if fails to allocate a new cluster.
 * 그렇지 않으면, 새로 할당된 클러스터의 클러스터 번호를 반환. */
cluster_t
fat_create_chain (cluster_t clst) {
	/* TODO: Your code goes here. */

	if (clst == 0) {
		//create new chain
		for (unsigned i = fat_fs->bs.fat_start; i <= fat_fs->fat_length; i++) {
			if (fat_fs->fat[i] == 0) {
				fat_put (i, EOChain);
				return i;
			}
		}
	} else { // 중간 인덱스를 넣었을때도 고려해야 할까?
		cluster_t temp;
		for (unsigned i = fat_fs->bs.fat_start; i <= fat_fs->fat_length; i++) {
			if (fat_fs->fat[i] == 0) {
				temp = fat_fs->fat[clst];
				fat_put (clst, i);
				fat_put (i, temp);
				return i;
			}
		}
	}
	return 0;
}

/* Remove the chain of clusters starting from CLST.
 * CLST에서 시작하는 클러스터 체인을 제거합니다.
 * pclst는 체인의 직접 이전 클러스터여야 합니다.
 * 이것은 이 함수를 실행한 후 pclst가 업데이트된 체인의 마지막 요소여야 함을 의미한다.
 * If PCLST is 0, assume CLST as the start of the chain. */
void
fat_remove_chain (cluster_t clst, cluster_t pclst) {
	/* TODO: Your code goes here. */

	cluster_t cur_clst = fat_fs->fat[clst];
	if (cur_clst == EOChain) {
		cur_clst = 0;
		pclst = 0;
	} else {
		// 현재 클러스터를 가리키는 이전 클러스터 인덱스 찾기
		for (unsigned i = fat_fs->bs.fat_start; i <= fat_fs->fat_length; i++) {
			if (fat_fs->fat[i] == clst) {
				pclst = i;
				break;
			} 
		}
		// 현재 클러스터부터 체인 삭제하기
		cluster_t temp;
		while (cur_clst != EOChain) {
			temp = cur_clst;
			cur_clst = 0;
			cur_clst = fat_fs->fat[temp];
		}
		cur_clst = 0;
	}

	while (cur_clst != EOChain) {
		cur_clst = EOChain;
		cur_clst = fat_fs->fat[cur_clst];
	}
}

/* Update a value in the FAT table.
 * FAT의 각 항목은 체인의 다음 클러스터(존재하는 경우; 그렇지 않은 경우 EOChain)를
 * 가리키므로 연결을 업데이트하는 데 사용할 수 있습니다.*/
void
fat_put (cluster_t clst, cluster_t val) {
	/* TODO: Your code goes here. */

	fat_fs->fat[clst] = val;
}

/* Fetch a value in the FAT table.
 * 주어진 클러스터 clst가 가리키는 클러스터 번호를 반환합니다. */
cluster_t
fat_get (cluster_t clst) {
	/* TODO: Your code goes here. */

	return fat_fs->fat[clst];
}

/* Covert a cluster # to a sector number.
 * 클러스터 번호 clst를 해당 섹터 번호로 변환하고 섹터 번호를 반환합니다. */
disk_sector_t
cluster_to_sector (cluster_t clst) {
	/* TODO: Your code goes here. */

	return clst;
}
