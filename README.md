# Navigation Data Updater
<h4 align="center">A command line program to build download and convert FAA CIFP to X-Plane format.</h4>

Navigation Data Updater (NDU) is a command line program used to download FAA <a href="https://www.faa.gov/air_traffic/flight_info/aeronav/digital_products/cifp/download/">CIFP</a> data and convert it into X-Plane understandable format. NDU uses <a href="https://github.com/Microsoft/cpprestsdk">cpprestsdk</a>, <a href="https://github.com/gabime/spdlog">spdlog</a>, <a href="https://github.com/zeux/pugixml">pugixml</a> and <a href="https://github.com/rikyoz/bit7z">bit7z</a>.

The project also needs 7-Zip DLL to extract the data from downloaded update files, and <a href="https://developer.x-plane.com/article/navdata-in-x-plane-11/">convert424toxp11.exe</a> from X-Plane Developer Site.

Because the data is from FAA so the ouput will only usable for US domestic flights.

## Installation
### Windows
If you don't want to compile the program by yourself, you can head over the <a href="https://github.com/phuongtran7/Navigation_Data_Updater/releases">releases</a> tab a get a pre-compiled version.

1. Install cpprestsdk, spdlog and pugixml with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
    * `vcpkg install cpprestsdk`
    * `vcpkg install spdlog`
    * `vcpkg install pugixml`
2. Clone the project: `git clone https://github.com/phuongtran7/Navigation_Data_Updater.git`.
3. Download or build <a href="https://github.com/rikyoz/bit7z">bit7z</a> library and correct the link path in project.
4. Build the project.

## Usage
1. Copy `convert424toxp11.exe` and `7z.dll` into the same folder as compiled executable.
2. Start "Navigation_Data_Updater.exe"
3. Wait until finish and copy the entire content of `Ouput` folder to `{X-Plane Root}/Custom Data`. 
4. Start X-Plane.