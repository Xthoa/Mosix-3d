#include "mbr.h"
#include "vfs.h"
#include "exec.h"
#include "kheap.h"

Node* devd;

void make_parts(Node* dev){
    File* f = open_node(dev, 0);
    if(!f) return;
    Mbr* mbr = kheap_alloc(sizeof(Mbr));
    read(f, mbr, 1);
    close(f);
    if(mbr->magic != 0xaa55){
        kheap_free(mbr);
        return;
    }
    char *name, *pinc;
    int slen = kstrlen(dev->name);
    if(isdigit(dev->name[slen - 1])){
        name = kheap_alloc(slen + 3);
        strcpy(name, dev->name);
        name[slen] = 'p';
        name[slen + 1] = '0';
        pinc = name + slen + 1;
        name[slen + 2] = '\0';
    }
    else{
        name = kheap_alloc(slen + 2);
        strcpy(name, dev->name);
        name[slen] = '0';
        pinc = name + slen;
        name[slen + 1] = '\0';
    }
    for(int i = 0; i < 4; i++){
        Partition p = mbr->part[i];
        if(p.secs == 0) continue;
        GenericDisk* gd = kheap_alloc(sizeof(GenericDisk));
        memcpy(gd, dev->data, sizeof(GenericDisk));
        gd->secstart = p.lba;
        gd->seclen = p.secs;
        Node* n = create_subnode(devd, name, NODE_DEVICE | NODE_BLOCKDEV);
        n->data = gd;
        n->fops = dev->fops;

        *pinc ++;
    }
    kheap_free(name);
    kheap_free(mbr);
}

Export void entry(int status){
    if(status == DRIVER_EXIT) return;

    devd = path_walk("/run/dev").node;
    for(Node* c = devd->child; c; c = c->next){
        if(c->attr & NODE_BLOCKDEV) make_parts(c);
    }
}
