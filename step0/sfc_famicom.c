#include "sfc_famicom.h"
#include <assert.h>
#include <string.h>

// ����Ĭ��ROM
static sfc_ecode sfc_load_default_rom(void*, sfc_rom_info_t*);
// �ͷ�Ĭ��ROM
static sfc_ecode sfc_free_default_rom(void*, sfc_rom_info_t*);

// ����һ�����(SB)�ĺ���ָ������
typedef void(*sfc_funcptr_t)();

/// <summary>
/// StepFC: ��ʼ��famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="argument">The argument.</param>
/// <param name="interfaces">The interfaces.</param>
/// <returns></returns>
sfc_ecode sfc_famicom_init(
    sfc_famicom_t* famicom, 
    void* argument, 
    const sfc_interface_t* interfaces) {
    assert(famicom && "bad famicom");
    famicom->argument = argument;
    // ����Ĭ�Ͻӿ�
    famicom->interfaces.load_rom = sfc_load_default_rom;
    famicom->interfaces.free_rom = sfc_free_default_rom;
    // �������
    memset(&famicom->rom_info, 0, sizeof(famicom->rom_info));
    // �ṩ�˽ӿ�
    if (interfaces) {
        const int count = sizeof(*interfaces) / sizeof(interfaces->load_rom);
        // ������Ч�ĺ���ָ��
        // UB: C��׼����һ����֤sizeof(void*)��ͬsizeof(fp) (�Ƿ���ϵ)
        //     ��������������һ��sfc_funcptr_t
        sfc_funcptr_t* const func_src = (sfc_funcptr_t*)interfaces;
        sfc_funcptr_t* const func_des = (sfc_funcptr_t*)&famicom->interfaces;
        for (int i = 0; i != count; ++i) if (func_src[i]) func_des[i] = func_src[i];
    }
    // һ��ʼ�������ROM
    return famicom->interfaces.load_rom(argument, &famicom->rom_info);
    return SFC_ERROR_OK;
}

/// <summary>
/// StepFC: ����ʼ��famicom
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_famicom_uninit(sfc_famicom_t* famicom) {
    // �ͷ�ROM
    famicom->interfaces.free_rom(famicom->argument, &famicom->rom_info);
}


#include <stdio.h>
#include <stdlib.h>

/// <summary>
/// ����Ĭ�ϲ���ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode sfc_load_default_rom(void* arg, sfc_rom_info_t* info) {
    assert(info->data_prgrom == NULL && "FREE FIRST");
    FILE* const file = fopen("nestest.nes", "rb");
    // �ı�δ�ҵ�
    if (!file) return SFC_ERROR_FILE_NOT_FOUND;
    sfc_ecode code = SFC_ERROR_ILLEGAL_FILE;
    // ��ȡ�ļ�ͷ
    sfc_nes_header_t nes_header;
    if (fread(&nes_header, sizeof(nes_header), 1, file)) {
        // ��ͷ4�ֽ�
        union { uint32_t u32; uint8_t id[4]; } this_union;
        this_union.id[0] = 'N';
        this_union.id[1] = 'E';
        this_union.id[2] = 'S';
        this_union.id[3] = '\x1A';
        // �Ƚ������ֽ�
        if (this_union.u32 == nes_header.id) {
            const size_t size1 = 16 * 1024 * nes_header.count_prgrom16kb;
            const size_t size2 =  8 * 1024 * nes_header.count_chrrom_8kb;
            uint8_t* const ptr = (uint8_t*)malloc(size1 + size2);
            // �ڴ�����ɹ�
            if (ptr) {
                code = SFC_ERROR_OK;
                // TODO: ʵ��Trainer
                // ����Trainer����
                if (nes_header.control1 & SFC_NES_TRAINER) fseek(file, 512, SEEK_CUR);
                // �ⶼ���˾Ͳ����ҵ�������
                fread(ptr, size1 + size2, 1, file);

                // ��дinfo���ݱ���
                info->data_prgrom = ptr;
                info->data_chrrom = ptr + size1;
                info->count_prgrom16kb = nes_header.count_prgrom16kb;
                info->count_chrrom_8kb = nes_header.count_chrrom_8kb;
                info->mapper_number 
                    = (nes_header.control1 >> 4) 
                    | (nes_header.control2 & 0xF0)
                    ;
                info->vmirroring    = (nes_header.control1 & SFC_NES_VMIRROR) > 0;
                info->four_screen   = (nes_header.control1 & SFC_NES_4SCREEN) > 0;
                info->save_ram      = (nes_header.control1 & SFC_NES_SAVERAM) > 0;
                assert(!(nes_header.control1 & SFC_NES_TRAINER) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_VS_UNISYSTEM) && "unsupported");
                assert(!(nes_header.control2 & SFC_NES_Playchoice10) && "unsupported");
            }
            // �ڴ治��
            else code = SFC_ERROR_OUT_OF_MEMORY;
        }
        // �Ƿ��ļ�
    }
    fclose(file);
    return code;
}

/// <summary>
/// �ͷ�Ĭ�ϲ���ROM
/// </summary>
/// <param name="arg">The argument.</param>
/// <param name="info">The information.</param>
/// <returns></returns>
sfc_ecode sfc_free_default_rom(void* arg, sfc_rom_info_t* info) {
    // �ͷŶ�̬���������
    free(info->data_prgrom);
    info->data_prgrom = NULL;

    return SFC_ERROR_OK;
}