/* $Id: exec.c,v 1.2 2009-08-08 06:49:44 masamic Exp $ */

/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2001/05/23 11:22:07  masamic
 * First imported source code and docs
 *
 * Revision 1.9  1999/12/07  12:42:21  yfujii
 * *** empty log message ***
 *
 * Revision 1.9  1999/11/29  06:22:21  yfujii
 * The way of recording instruction history is changed.
 *
 * Revision 1.8  1999/11/01  06:23:33  yfujii
 * Some debugging functions are introduced.
 *
 * Revision 1.7  1999/10/28  06:34:08  masamichi
 * Modified trace behavior
 *
 * Revision 1.6  1999/10/26  02:12:07  yfujii
 * Fixed a bug of displaying code in the wrong byte order.
 *
 * Revision 1.5  1999/10/26  01:31:54  yfujii
 * Execution history and address trap is added.
 *
 * Revision 1.4  1999/10/22  03:23:18  yfujii
 * #include <dos.h> is removed.
 *
 * Revision 1.3  1999/10/20  02:43:59  masamichi
 * Add for showing more information about errors.
 *
 * Revision 1.2  1999/10/18  03:24:40  yfujii
 * Added RCS keywords and modified for WIN32 a little.
 *
 */

#undef	MAIN

#include <stdio.h>
#include <stdlib.h>
#include "run68.h"
#if defined(DOSX)
#include <dos.h>
#endif

/* prog_ptr_u�͕����t��char�ŕs�ւȂ̂ŁA�����Ȃ�char�ɕϊ����Ă����B*/
#define prog_ptr_u ((unsigned char *)prog_ptr)
void	run68_abort( long );
extern char *disassemble(long addr, long* next_addr);

/*
 �@�@�\�F1���ߎ��s����
 �߂�l�F TRUE = ���s�I��
         FALSE = ���s�p��
*/
int prog_exec()
{
	char	*pc_ptr;
	long	adr;
	short	save_s;

	/* ���4�r�b�g�Ŗ��߂�U�蕪���� */
	pc_ptr = prog_ptr + pc;
	switch( *pc_ptr & 0xF0 ) {
		case 0x00:
			return( line0( pc_ptr ) );
		case 0x10:
		case 0x20:
		case 0x30:
			return( line2( pc_ptr ) );
		case 0x40:
			return( line4( pc_ptr ) );
		case 0x50:
			return( line5( pc_ptr ) );
		case 0x60:
			return( line6( pc_ptr ) );
		case 0x70:
			return( line7( pc_ptr ) );
		case 0x80:
			return( line8( pc_ptr ) );
		case 0x90:
			return( line9( pc_ptr ) );
		case 0xB0:
			return( lineb( pc_ptr ) );
		case 0xC0:
			return( linec( pc_ptr ) );
		case 0xD0:
			return( lined( pc_ptr ) );
		case 0xE0:
			return( linee( pc_ptr ) );
		case 0xF0:
			return( linef( pc_ptr ) );
		case 0xA0:
			save_s = SR_S_REF();
			SR_S_ON();
			adr = mem_get( 0x28, S_LONG );
			if ( adr != HUMAN_WORK ) {
				ra [ 7 ] -= 4;
				mem_set( ra [ 7 ], pc, S_LONG );
				ra [ 7 ] -= 2;
				mem_set( ra [ 7 ], sr, S_WORD );
				pc = adr;
				return( FALSE );
			}
			if ( save_s == 0 )
				SR_S_OFF();
			pc += 2;
			err68( "A�n�񊄂荞�݂����s���܂���" );
			return( TRUE );
		default:
			pc += 2;
			err68( "�������Ȗ��߂����s���܂���" );
			return( TRUE );
	}
}

/*
 �@�@�\�F�R���f�B�V�������������Ă��邩�ǂ������ׂ�
 �߂�l�F TRUE = ����
 �@�@�@�@FALSE = �s����
*/
int get_cond( char cond )
{
	switch( cond ) {
		case 0x00:	/* t */
			return( TRUE );
		case 0x02:	/* hi */
			if ( CCR_C_REF() == 0 && CCR_Z_REF() == 0 )
				return( TRUE );
			break;
		case 0x03:	/* ls */
			if ( CCR_C_REF() != 0 || CCR_Z_REF() != 0 )
				return( TRUE );
			break;
		case 0x04:	/* cc */
			if ( CCR_C_REF() == 0 )
				return( TRUE );
			break;
		case 0x05:	/* cs */
			if ( CCR_C_REF() != 0 )
				return( TRUE );
			break;
		case 0x06:	/* ne */
			if ( CCR_Z_REF() == 0 )
				return( TRUE );
			break;
		case 0x07:	/* eq */
			if ( CCR_Z_REF() != 0 )
				return( TRUE );
			break;
		case 0x08:	/* vc */
			if ( CCR_V_REF() == 0 )
				return( TRUE );
			break;
		case 0x09:	/* vs */
			if ( CCR_V_REF() != 0 )
				return( TRUE );
			break;
		case 0x0A:	/* pl */
			if ( CCR_N_REF() == 0 )
				return( TRUE );
			break;
		case 0x0B:	/* mi */
			if ( CCR_N_REF() != 0 )
				return( TRUE );
			break;
		case 0x0C:	/* ge */
			if ( (CCR_N_REF() != 0 && CCR_V_REF() != 0) ||
			     (CCR_N_REF() == 0 && CCR_V_REF() == 0) )
				return( TRUE );
			break;
		case 0x0D:	/* lt */
			if ( (CCR_N_REF() != 0 && CCR_V_REF() == 0) ||
			     (CCR_N_REF() == 0 && CCR_V_REF() != 0) )
				return( TRUE );
			break;
		case 0x0E:	/* gt */
			if ( CCR_Z_REF() == 0 &&
			   ( (CCR_N_REF() != 0 && CCR_V_REF() != 0) ||
			     (CCR_N_REF() == 0 && CCR_V_REF() == 0) ) )
				return( TRUE );
			break;
		case 0x0F:	/* le */
			if ( CCR_Z_REF() != 0 ||
			     (CCR_N_REF() != 0 && CCR_V_REF() == 0) ||
			     (CCR_N_REF() == 0 && CCR_V_REF() != 0) )
				return( TRUE );
			break;
	}

	return( FALSE );
}

/*
 �@�@�\�F���s���G���[���b�Z�[�W��\������
 �߂�l�F�Ȃ�
*/
void	err68( char *mes )
{
    OPBuf_insert(&OP_info);
	fprintf(stderr, "run68 exec error: %s PC=%06X\n", mes, pc);
	if ( memcmp( mes, "����`", 6 ) == 0 )
		fprintf(stderr, "code = %08X\n",mem_get( pc - 4, S_LONG ));
    OPBuf_display(10);
    run68_abort(pc);
}

/*
 �@�@�\�F���s���G���[���b�Z�[�W��\������(����2)
   �����F
	char*	mes	<in>	���b�Z�[�W
	char*	file	<in>	�t�@�C����
	int	line	<in>	�s�ԍ�
 �߂�l�F�Ȃ�
*/
void err68a( char *mes, char *file, int line )
{
    OPBuf_insert(&OP_info);
	fprintf(stderr, "run68 exec error: %s PC=%06X\n", mes, pc);
	fprintf(stderr, "\tAt %s:%d\n", file, line);
	if ( memcmp( mes, "����`", 6 ) == 0 )
		fprintf(stderr, "code = %08X\n",mem_get( pc - 4, S_LONG ));
    OPBuf_display(10);
    run68_abort(pc);
}

/*
   �@�\�F���s���G���[���b�Z�[�W��\������(����3)
   �����F
    char*  mes	<in>    ���b�Z�[�W
    long   pc   <in>    �v���O�����J�E���^
    long   ppc  <in>    ��O�Ɏ��s�������߂̃v���O�����J�E���^
   �߂�l�F
    �Ȃ�
*/
void err68b(char *mes, long pc, long ppc)
{
    OPBuf_insert(&OP_info);
	fprintf(stderr, "run68 exec error: %s PC=%06X\n", mes, pc);
	fprintf(stderr, "PC of previous op code: PC=%06X\n", ppc);
	if ( memcmp( mes, "����`", 6 ) == 0 )
		fprintf(stderr, "code = %08X\n",mem_get( pc - 4, S_LONG ));
    OPBuf_display(10);
    run68_abort(pc);
}

/*
 �@�@�\�F�A�h���X���W�X�^���C���N�������g����
 �߂�l�F�Ȃ�
*/
void inc_ra( char reg, char size )
{
	if ( reg == 7 && size == S_BYTE ) {
		ra [ 7 ] += 2;
	} else {
		switch( size ) {
			case S_BYTE:
				ra [ reg ] += 1;
				break;
			case S_WORD:
				ra [ reg ] += 2;
				break;
			default:	/* S_LONG */
				ra [ reg ] += 4;
				break;
		}
	}
}

/*
 �@�@�\�F�A�h���X���W�X�^���f�N�������g����
 �߂�l�F�Ȃ�
*/
void dec_ra( char reg, char size )
{
	if ( reg == 7 && size == S_BYTE ) {
		ra [ 7 ] -= 2;
	} else {
		switch( size ) {
			case S_BYTE:
				ra [ reg ] -= 1;
				break;
			case S_WORD:
				ra [ reg ] -= 2;
				break;
			default:	/* S_LONG */
				ra [ reg ] -= 4;
				break;
		}
	}
}

/*
 �@�@�\�F�e�L�X�g�J���[��ݒ肷��
 �߂�l�F�Ȃ�
*/
void text_color( short c )
{
	switch( c ) {
		case  0:
			printf("%c[0;30m", 0x1B );
			break;
		case  1:
			printf("%c[0;36m", 0x1B );
			break;
		case  2:
			printf("%c[0;33m", 0x1B );
			break;
		case  3:
			printf("%c[0;37m", 0x1B );
			break;
		case  4:
			printf("%c[0;1;30m", 0x1B );
			break;
		case  5:
			printf("%c[0;1;36m", 0x1B );
			break;
		case  6:
			printf("%c[0;1;33m", 0x1B );
			break;
		case  7:
			printf("%c[0;1;37m", 0x1B );
			break;
		case  8:
			printf("%c[0;30;40m", 0x1B );
			break;
		case  9:
			printf("%c[0;30;46m", 0x1B );
			break;
		case 10:
			printf("%c[0;30;43m", 0x1B );
			break;
		case 11:
			printf("%c[0;30;47m", 0x1B );
			break;
		case 12:
			printf("%c[0;30;1;40m", 0x1B );
			break;
		case 13:
			printf("%c[0;30;1;46m", 0x1B );
			break;
		case 14:
			printf("%c[0;30;1;43m", 0x1B );
			break;
		case 15:
			printf("%c[0;30;1;47m", 0x1B );
			break;
	}
}

/*
   �@�\�F�J�[�\���ʒu�𓾂�
 �߂�l�F�J�[�\���ʒu
*/
long get_locate()
{
	UShort x = 0, y = 0;

#if defined(WIN32)
	// @Todo
#elif defined(DOSX)
	union	REGS inreg, outreg;
	short	save_s;

	fflush( stdout );
	inreg.h.ah = 0x03;
	inreg.h.bh = 0;
	int86( 0x10, &inreg, &outreg );
	x = outreg.h.dl;
	y = outreg.h.dh;
	save_s = SR_S_REF();
	SR_S_ON();
	mem_set( 0x974, x, S_WORD );
	mem_set( 0x976, y, S_WORD );
	if ( save_s == 0 )
		SR_S_OFF();
#endif

	return( (x << 16) | y );
}

/*
   ���ߏ�񃊃��O�o�b�t�@�̍�Ɨ̈�
*/
#define MAX_OPBUF 200
static int num_entries;
static int current_p;
static EXEC_INSTRUCTION_INFO entry[MAX_OPBUF];
/*
   �@�\�F
     ���s�������߂̏��������O�o�b�t�@�ɕۑ�����B
   �p�����[�^�F
     EXEC_INSTRUCTION_INFO  op <in>  ���ߏ��
   �߂�l�F
     �Ȃ��B
*/
void OPBuf_insert(const EXEC_INSTRUCTION_INFO *op)
{
    if (num_entries < MAX_OPBUF)
    {
        num_entries ++;
    }
    entry[current_p++] = *op;
    if (MAX_OPBUF == current_p)
    {
        current_p = 0;
    }
}

/*
   �@�\�F
     ���ߏ�񃊃��O�o�b�t�@���N���A����B
   �p�����[�^�G
     �Ȃ��B
   �߂�l�F
     �Ȃ��B
*/
void OPBuf_clear()
{
    num_entries = 0;
    current_p = 0;
}

/*
   �@�\�F
     ���ߏ�񃊃��O�o�b�t�@�̃T�C�Y���擾����B
     �p�����[�^�F
   �Ȃ��B
     �߂�l�F
   int  �o�b�t�@�̃G���g����
*/
int OPBuf_numentries()
{
    return num_entries;
}

/*
    �@�\�F
      ���ߏ�񃊃��O�o�b�t�@��no�Ԗڂ̃G���g�����擾����B
    �p�����[�^�F
      int   no  <in>   ���o�������G���g���ԍ�(0���ŋ߂̂���)
    �߂�l�F
      EXEC_INSTRUCTION_INFO*  ���ߏ��ւ̃|�C���^
*/
const EXEC_INSTRUCTION_INFO *OPBuf_getentry(int no)
{
    int p;
    if (no < 0 || num_entries <= no)
        return NULL;
    p = current_p - no - 1;
    if (p < 0)
    {
        p += MAX_OPBUF;
    }
    return &entry[p];
}

/*
    �@�\�F
      ���ߏ�񃊃��O�o�b�t�@�̓��e���o�͂���B
    �p�����[�^�F
      int  n   <in>  �\������o�b�t�@�̃G���g����
    �߂�l�F
      �Ȃ��B
*/
void OPBuf_display(int n)
{
    int max = OPBuf_numentries();
    int i;
    if (max < n)
        n = max;
    fprintf(stderr, "** EXECUTED INSTRUCTION HISTORY **\n");
    fprintf(stderr, "ADDRESS OPCODE                    MNEMONIC\n");
    fprintf(stderr, "-------------------------------------------------------\n");
    for (i = n-1; 0 <= i; i --)
    {
        const EXEC_INSTRUCTION_INFO *op;
        long addr, naddr;
        char *s, hex[64];
        unsigned short code;
        int j;

        op = OPBuf_getentry(i);
        addr = op->pc;
        s = disassemble(addr, &naddr);
        sprintf(hex, "$%06X ", addr);
        while (addr < naddr)
        {
            char *p = hex + strlen(hex);
            code = (((unsigned short)prog_ptr_u[addr]) << 8) + (unsigned short)prog_ptr_u[addr + 1];
            sprintf(p, "%04X ", code);
            addr += 2;
        }
        for (j = strlen(hex); j < 34; j ++)
        {
            hex[j] = ' ';
        }
        hex[j] = '\0';
        if (s == NULL)
        {
            fprintf(stderr, "%s%s\n", hex, "????");
        } else
        {
            fprintf(stderr, "%s%s\n", hex, s);
        }
    }
}

/*
 �@�@�\�FPC�̎w������������C���f�b�N�X���W�X�^�{8�r�b�g�f�B�X�v���[�X�����g
 �@�@�@�@�̒l�𓾂�
 �߂�l�F���̒l
*/
int get_idx(int *pc, char *regstr)
{
	char	*mem;
	char	idx2;
	char	idx_reg;

	mem = prog_ptr + (*pc);

	idx2 = *(mem++);
	idx_reg = ((idx2 >> 4) & 0x07);
	if ( (idx2 & 0x80) == 0 ) {
            sprintf(regstr, "d%d", idx_reg);
	} else {
            sprintf(regstr, "d%d", idx_reg);
        }
	if ( (idx2 & 0x08) == 0 ) {	/* WORD */
            strcat(regstr, ".w");
        } else {
            strcat(regstr, ".l");
	}
	(*pc) += 2;

	return ((int)(*mem));
}

/*
 �@�@�\�FPC�̎w������������w�肳�ꂽ�T�C�Y�̃C�~�f�B�G�C�g�f�[�^���Q�b�g���A
 �@�@�@�@�T�C�Y�ɉ�����PC��i�߂�
 �߂�l�F�f�[�^�̒l
*/
long get_imi(int *pc, char size )
{
	UChar	*mem;
	long	d;

	mem = (UChar *)prog_ptr + (*pc);


	switch( size ) {
		case S_BYTE:
			(*pc) += 2;
			return( *(mem + 1) );
		case S_WORD:
			(*pc) += 2;
			d = *(mem++);
			d = ((d << 8) | *mem);
			return( d );
		default:	/* S_LONG */
			(*pc) += 4;
			d = *(mem++);
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *(mem++));
			d = ((d << 8) | *mem);
			return( d );
	}
}

/*
    �@�\�F
      �I�y�����h������𐶐�����B
    �p�����[�^�F
      char *buf           <out>    ����������������i�[����B
      int  AddressingMode <in>     �A�h���b�V���O���[�h
      int  RegisterNumber <in>     ���W�X�^�ԍ��i�܂��̓A�h���b�V���O���[�h�j
      char *pc            <in/out> �g�����擾�p�v���O�����J�E���^
    �߂�l�F
      �Ȃ��B
*/

void get_operand(char *buf, int *pc, int AddressingMode, int RegisterNumber, int size)
{
    char regstr[16];
    int  disp;

    switch (AddressingMode) {
        case 0:
            sprintf(buf, "d%d", RegisterNumber);
            break;
        case 1:
            sprintf(buf, "a%d", RegisterNumber);
            break;
        case 2:
            sprintf(buf, "(a%d)", RegisterNumber);
            break;
        case 3:
            sprintf(buf, "(a%d)+", RegisterNumber);
            break;
        case 4:
            sprintf(buf, "-(a%d)", RegisterNumber);
            break;
        case 5:
            disp = get_imi(pc, S_WORD);
            sprintf(buf, "$%04x(a%d)", disp, RegisterNumber); 
            break;
        case 6:
            disp = get_idx(pc, regstr);
            sprintf(buf, "%d(a%d,%s)", disp, RegisterNumber, regstr); 
            break;
        case 7:
            switch( RegisterNumber ) {
                case 0:
                    disp = get_imi(pc, S_WORD);
                    sprintf(buf, "$%04x", disp); 
                    break;
                case 1:
                    disp = get_imi(pc, S_LONG);
                    sprintf(buf, "$%08x", disp); 
                    break;
                case 2:
                    disp = get_imi(pc, S_WORD);
                    sprintf(buf, "$%04x(pc)", disp); 
                    break;
                case 3:
                    disp = get_idx(pc, regstr);
                    sprintf(buf, "%d(pc,%s)", disp, regstr); 
                    break;
                case 4:
                    disp = get_imi(pc, size);
                    switch (size) {
                        case S_BYTE:
                            sprintf(buf, "#$%02x", disp); 
                            break;
                        case S_WORD:
                            sprintf(buf, "#$%04x", disp); 
                            break;
                        case S_LONG:
                            sprintf(buf, "#$%08x", disp); 
                            break;
                        default:
                            strcpy(buf, "????????"); 
                    }
                    break;
                default:
                    strcpy(buf, "????????"); 
            }
            break;
    }
}

