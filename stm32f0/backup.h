#ifndef BACKUP_H_INCLUDED
#define BACKUP_H_INCLUDED

enum BackupRegister {
    BKP0 = 0,
    BKP1,
    BKP2,
    BKP3,
    BKP4
};

extern void backup_write(enum BackupRegister reg, uint32_t value);
extern uint32_t backup_read(enum BackupRegister reg);

#endif
