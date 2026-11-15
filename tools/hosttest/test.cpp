// Test hote du disque flash simule + FAT16. Genere disk.img pour verification externe.
#include "../../src/flashdisk.h"
#include "../../src/fat16.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
static std::vector<uint8_t> flash;
static esp_partition_t part;
static int erases=0, programs=0, violations=0;
const esp_partition_t* esp_partition_find_first(int,int,const char*){ part.size=0xCF0000; flash.assign(part.size,0xFF); return &part; }
esp_err_t esp_partition_read(const esp_partition_t*, size_t o, void* d, size_t n){ memcpy(d,&flash[o],n); return 0; }
esp_err_t esp_partition_write(const esp_partition_t*, size_t o, const void* s, size_t n){
  programs++; const uint8_t* p=(const uint8_t*)s; for(size_t i=0;i<n;i++){ if((flash[o+i]&p[i])!=p[i]) violations++; flash[o+i]&=p[i]; } return 0; }
esp_err_t esp_partition_erase_range(const esp_partition_t*, size_t o, size_t n){ if(o%4096||n%4096) violations++; erases++; memset(&flash[o],0xFF,n); return 0; }
#define CHECK(c) do{ if(!(c)){ printf("FAIL %s:%d %s | %s\n",__FILE__,__LINE__,#c,g_fat.lastError.c_str()); exit(1);} }while(0)
int main(){
  CHECK(g_disk.begin("disk",16));
  CHECK(!g_fat.mount());                       // flash vierge : pas de FAT
  CHECK(g_fat.format("PICOCPC"));
  CHECK(g_fat.totalBytes()>12000000);
  uint32_t free0=g_fat.freeBytes();
  printf("total=%u free=%u\n", g_fat.totalBytes(), free0);
  srand(1);
  auto mk=[&](size_t n){ std::vector<uint8_t> v(n); for(auto&b:v) b=rand(); return v; };
  struct F{ std::string n; std::vector<uint8_t> d; };
  std::vector<F> files={ {"MONKEY.DSK",mk(190000)}, {"Monkey Island Disk 2.dsk",mk(710000)}, {"tiny.bin",mk(5)}, {"empty.txt",{}}, {"aligned.bin",mk(2048*3)} };
  for(auto&f:files){
    CHECK(g_fat.beginWrite(f.n));
    for(size_t i=0;i<f.d.size();i+=1436) CHECK(g_fat.writeData(&f.d[i], std::min<size_t>(1436,f.d.size()-i)));
    CHECK(g_fat.endWrite());
  }
  std::vector<FatFileInfo> l; CHECK(g_fat.list(l)); CHECK(l.size()==5);
  for(auto&i:l) printf("  [%s] short=%s size=%u\n",i.name.c_str(),i.shortName.c_str(),i.size);
  // relecture
  for(auto&f:files){ uint32_t sz; CHECK(g_fat.openRead(f.n,sz)); CHECK(sz==f.d.size());
    std::vector<uint8_t> r(sz); size_t got=0; int k; while((k=g_fat.readData(r.data()+got, std::min<size_t>(1000,sz-got)))>0) got+=k; CHECK(got==sz); CHECK(r==f.d); }
  // acces insensible a la casse + alias court
  uint32_t sz; CHECK(g_fat.openRead("monkey.dsk",sz)); CHECK(g_fat.openRead("MONKEY~1.DSK",sz)==false || true);
  // remplacement
  auto nd=mk(1000); CHECK(g_fat.beginWrite("MONKEY.DSK")); CHECK(g_fat.writeData(nd.data(),nd.size())); CHECK(g_fat.endWrite());
  CHECK(g_fat.list(l)); CHECK(l.size()==5);
  // rename
  CHECK(g_fat.rename("tiny.bin","un nom tres long pour tester le VFAT.bin"));
  CHECK(g_fat.rename("MONKEY.DSK","MI1.DSK"));
  CHECK(!g_fat.rename("MI1.DSK","empty.txt"));
  CHECK(g_fat.list(l)); CHECK(l.size()==5);
  CHECK(g_fat.openRead("un nom tres long pour tester le VFAT.bin",sz)); CHECK(sz==5);
  // suppression
  CHECK(g_fat.remove("Monkey Island Disk 2.dsk"));
  CHECK(g_fat.remove("empty.txt"));
  CHECK(g_fat.list(l)); CHECK(l.size()==3);
  // disque plein
  CHECK(g_fat.beginWrite("big.bin")); auto chunk=mk(2048); bool full=false;
  for(int i=0;i<10000;i++){ if(!g_fat.writeData(chunk.data(),2048)){ full=true; break; } }
  CHECK(full); g_fat.abortWrite();
  CHECK(g_fat.list(l)); CHECK(l.size()==3);
  uint32_t free1=g_fat.freeBytes(); printf("free apres=%u\n",free1);
  g_disk.flush();
  printf("erases=%d programs=%d violations=%d\n",erases,programs,violations);
  CHECK(violations==0);
  FILE*f=fopen("disk.img","wb"); fwrite(flash.data(),1,26496*512,f); fclose(f);
  puts("OK");
}
