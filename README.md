# SuperNova Engine

## Build

1. Поставить [python 3.9.5](https://www.python.org/downloads/release/python-395/) При установке прописать его в PATH. Убедится, что питон ставится в `C:\Program Files`

2. Запустить консоль от одмена

   ```
   python --version  # Проверить установку python
   pip install wheel
   pip install conan
   conan --version   # Проверить установку conan
   ```

3. Разбираться с одновременной конфигурацией CMakePresets для Ninja и VisualStudio NMake долго, поэтому пока для билда нужен Ninja
   - Берем [ninja.exe](https://github.com/ninja-build/ninja/releases)
   - Прописываем в PATH путь до папки с ним

4. Инструкция для disabled people (юзеров CLion)
   - Поставить CLion 2021.2
   - Поставить conan plugin
   - Открыть папку с проектом в CLion
   - Пидорас откроект добавление билд конфигураций, убираем дефолтные, которые он надобавлял, закрываем окно, если он успеет создать папку с билдом аля `cmake-build-debug`, удаляем ее нахуй
   - Жмем Ctrl+Shift+A/⇧⌘A (Find Action), пишем `Load Cmake Presets`, выбираем Debug, повторяем для Release
   - Ждем пока CLion сконфигурит билд конфигурации и conan поставит пакеты
   - Advanced манипуляции на этом должны закончится