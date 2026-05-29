# /proc Tabanlı Sistem İzleme Aracı

Linux `/proc` dosya sistemini okuyarak CPU, bellek ve süreç bilgilerini raporlayan C uygulaması.

## Derleme

```bash
make
```

## Kullanım

```bash
# Tek seferlik anlık rapor
./monitor

# 3 saniyede bir sürekli izleme
./monitor -c -i 3

# Sürekli izleme + dosyaya kayıt
./monitor -c -i 5 -o rapor.txt

# Yardım
./monitor -h
```

## Seçenekler

| Seçenek      | Açıklama                        | Varsayılan |
|--------------|---------------------------------|------------|
| `-i <sn>`    | Güncelleme aralığı (saniye)     | 2          |
| `-c`         | Sürekli izleme modu             | kapalı     |
| `-o <dosya>` | Her güncellemede dosyaya yaz    | —          |
| `-h`         | Yardım                          | —          |

## Mimari

```
main.c         → argüman parse, sinyal, ana döngü
collector.c    → ayrı pthread'de /proc okuma döngüsü
proc_reader.c  → /proc/stat, /proc/meminfo, /proc/<pid>/stat
display.c      → terminale tablo çıktısı
report.c       → dosyaya rapor kaydetme
utils.c        → qsort karşılaştırıcılar
monitor.h      → ortak tanımlar
```

## Karşılanan Gereksinimler

- [x] `/proc` üzerinden CPU, bellek ve süreç bilgisi okuma
- [x] En çok CPU kullanan süreçleri listeleme (Top 10)
- [x] En çok bellek kullanan süreçleri listeleme (Top 10)
- [x] Belirli aralıklarla yenilenen izleme modu (`-c -i <sn>`)
- [x] Verileri dosyaya rapor olarak kaydetme (`-o`)
- [x] Veri toplama işlemi ayrı thread ile (`pthread`)
- [x] Paylaşılan veriler mutex ile senkronize
- [x] Hatalı PID / erişilemeyen dosyalarda güvenli hata yönetimi