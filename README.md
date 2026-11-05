# stm32-bno055-digital-compass-driver

# STM32 ve BNO055 ile Dijital Pusula

Bu projede BNO055 sensörünü **AMG modunda** çalıştırarak STM32F401RE üzerinde dijital bir pusula yaptık. İvmeölçer, manyetometre ve jiroskop verilerini I²C üzerinden 20 ms aralıklarla okuyoruz. STM32, kartın eğimini hesaba katarak pusula yönünü hesaplıyor ve bu yönü jiroskop ölçümleriyle birleştiriyor.

## Bağlantılar ve CubeMX ayarları

STM32CubeMX'te **Connectivity → I2C1** altında `I2C` modunu seçin ve haberleşme hızını **100 kHz** olarak ayarlayın.

| STM32F401RE | CubeMX ayarı | BNO055 modülü |
| --- | --- | --- |
| PB6 | `I2C1_SCL` | SCL |
| PB7 | `I2C1_SDA` | SDA |
| PA5 | `GPIO_Output` | RST |
| GND | — | GND |

PA5 için **User Label** alanına `BNO_RST` yazın ve kodu oluşturun. Modülü kendi kartının besleme gereksinimine göre besleyin. Sürücü varsayılan olarak `0x28` adresini kullanır. Modülünüz `0x29` adresindeyse `BNO055_Driver.h` içindeki `BNO055_I2C_address` değerini `(0x29 << 1)` olarak değiştirin.

## Sürücüyü projeye ekleme

1. `BNO055_Driver.h` ve `MadgwickAHRS.h` dosyalarını projenizin `Core/Inc` klasörüne kopyalayın.
2. `BNO055_Driver.c` ve `MadgwickAHRS.c` dosyalarını `Core/Src` klasörüne kopyalayın.
3. Kendi `main.c` dosyanızda `USER CODE BEGIN Includes` bölümüne `#include "BNO055_Driver.h"` satırını ekleyin.
4. Bu depodaki `main.c` dosyasından aşağıdaki üç bölümün içeriğini kendi dosyanızdaki aynı bölümlere kopyalayın.

| `main.c` bölümü | Görevi |
| --- | --- |
| `USER CODE BEGIN PV` | Sensör, filtre ve zamanlama değişkenlerini tanımlar. |
| `USER CODE BEGIN 2` | Sürücüyü `hi2c1` ile eşleştirir, sensörü başlatır ve ölçüm periyodunu ayarlar. |
| `USER CODE BEGIN 3` | Sensör verilerini okur, jiroskop kaymasını düzeltir ve pusula yönünü günceller. |

## Jiroskop kaymasını nasıl azaltıyoruz?

Kart sabitken bile jiroskop küçük bir açısal hız ölçebilir. Bu hız zamanla toplandığında, kart dönmediği hâlde hesaplanan açı değişir. Sürücü, **200 örneklik** pencerelerde ivme ve jiroskop ölçümlerini kontrol eder. Kartın sabit kaldığını tespit ettiğinde her eksenin ortalama açısal hızına göre ofsetini günceller. Sonraki ölçümlerden bu ofseti çıkarır ve kalan hızın büyüklüğü 0,1 °/s'nin altındaysa değeri sıfır kabul eder.

## Manyetometre kalibrasyonu

- **Hard iron — ofset düzeltmesi.** Kartın yakınındaki sabit bir manyetik kaynak, ölçümleri bir yöne kaydırabilir. Kart farklı yönlere çevrildiğinde her eksende görülen en küçük ve en büyük değerin orta noktası, o eksenin ofsetini verir. Sürücü bu ofseti ölçümden çıkarır.
- **Soft iron — eksen ölçeği düzeltmesi.** Yakındaki metal parçalar manyetik alanı bozduğunda bazı eksenlerde daha geniş bir ölçüm aralığı oluşabilir. Sürücü, her eksenin ölçüm aralığını kullanarak ayrı bir ölçek katsayısı hesaplar ve ofseti düzeltilmiş ölçüme uygular.

Örnek uygulama, `sensor_init()` içinde kayıtlı kalibrasyon değerleriyle başlar. Pusulayı kendi donanımınıza uyarlarken ofsetleri ve eksenlerin minimum/maksimum değerlerini sensör son montaj konumundayken aldığınız ölçümlerle güncelleyin.

## Filtreler ve yön hesabı

1. **Ölçümleri yumuşatma.** İvmeölçer ve manyetometre verilerine alçak geçiren filtre uygulanır. Filtre, 50 Hz örnekleme ve 5 Hz kesim frekansına göre ayarlanmıştır.
2. **Eğim telafisi.** STM32, jiroskopla kartın yatış ve yunuslama açılarını takip eder. Ölçülen ivme yerçekimine yakınken bu açıları ivmeölçerle düzeltir. Ardından manyetik alanı yatay düzleme yansıtarak kart eğikken de pusula yönünü hesaplar.
3. **Yönü birleştirme.** Tamamlayıcı filtre, dönüşleri jiroskopla takip eder ve biriken yön hatasını manyetometreyle kademeli olarak düzeltir. Düzeltilmiş manyetik alan büyüklüğü kodda ayarlanan 43,5–48,5 µT aralığının dışına çıktığında manyetometre düzeltmesi devre dışı kalır.

## Sonuçları izleme

Projeyi STM32CubeIDE ile derleyip karta yükleyin. Kartı başlangıçta birkaç saniye sabit tutun. **Debug → Live Expressions** görünümünde şu değişkenleri izleyebilirsiniz:

| Değişken | Anlamı |
| --- | --- |
| `sensor_info.who_i_am` | Sensör kimliği. BNO055 için beklenen değer `0xA0`. |
| `sensor_info.roll` ve `sensor_info.pitch` | Yatış ve yunuslama açıları, radyan. |
| `sensor_info.D` | Eğim telafisi uygulanmış pusula yönü, derece. |
| `sensor_info.yaw` | Jiroskop ve pusula ölçümlerinden elde edilen filtrelenmiş yön, derece. |

Kartı düz tutup dikey ekseni etrafında çevirdiğinizde `D` ve `yaw` değerlerini karşılaştırabilirsiniz. `yaw` başlangıçta sıfırda kalıyorsa `sensor_info.mag_gecici` değerini kontrol edin. Filtre, yön takibine başlamak için manyetik alanın art arda kabul aralığında ölçülmesini bekler.
