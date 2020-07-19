#pragma once

#include "part.h"
#include "vm_declarations.h"

#define PMT_SIZE 192  //Size of page map table in pages
#define PMT_ENTRY_NUM 16384  //broj deskriptora u pmt = 2^14
#define OFFSET 10
#define OFFSET_MASK 1023
//Maske za izdvajanje odredjenih bita deskriptora
#define S_BIT 1  //Segment allocated bit
#define SEGSTART_BIT 2
#define SEGEND_BIT 4
#define V_BIT 8
#define D_BIT 16
#define READ_BIT 32
#define WRITE_BIT 64
#define EXEC_BIT 128

typedef struct {
	int bits;
	PageNum vmPage;
	ClusterNo cluster;
} descriptor;

