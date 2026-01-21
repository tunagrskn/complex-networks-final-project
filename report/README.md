# IEEE Conference Paper - Leader Election Algorithms Report

Bu klasör, Dağıtık Algoritma Tasarım ve Analizi dersi final projesi için IEEE formatında hazırlanmış raporu içermektedir.

## Dosyalar

- `main.tex` - Ana LaTeX kaynak dosyası
- `Makefile` - Derleme komutları

## Derleme

### Gereksinimler
- TeXLive veya MiKTeX dağıtımı
- pdflatex
- IEEEtran belge sınıfı (genellikle standart dağıtımlarda bulunur)

### Komutlar

```bash
# PDF oluşturmak için
make

# veya doğrudan
pdflatex main.tex
bibtex main
pdflatex main.tex
pdflatex main.tex

# Temizlik
make clean
```

## Rapor İçeriği

Rapor aşağıdaki bölümlerden oluşmaktadır:

1. **Giriş** - Lider seçim probleminin tanımı ve motivasyon
2. **Teorik Arka Plan** - Senkron modeller ve algoritma temelleri
3. **Algoritma Tasarımı** - Her iki algoritmanın detaylı açıklaması
4. **Gerçekleme** - OMNeT++ simülasyon ortamı ve modül yapısı
5. **Deneysel Sonuçlar** - Performans ölçümleri ve karşılaştırma
6. **Tartışma** - Sonuçların analizi ve ölçeklenebilirlik
7. **Sonuç** - Özet ve gelecek çalışmalar

## IEEE Formatı

Rapor, IEEE konferans makalesi formatında hazırlanmıştır. Şablon için:
https://www.ieee.org/conferences/publishing/templates
