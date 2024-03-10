# pword
`pword` is a simple yet powerful password generator designed for [Flipper Zero](https://flipperzero.one/) devices. It allows users to generate random, strong, and customizable passwords on the go.

## How It Works

Upon launching the app, the user is presented with a menu to select the desired action:
1. Generate a customizable random password
2. View the About page

![1p](https://raw.githubusercontent.com/dagnazty/Flipper_Apps/main/pword/images/4.png)

The application utilizes the device's hardware buttons for navigation:
- **Up/Down** arrows to switch between different character sets
- **OK** button to generate a new password
- **Left/Right** arrows to adjust the password length
- **Back** to return to the previous menu or exit the app
- The generated password will also be printed to the client log for retrieval

![2p](https://raw.githubusercontent.com/dagnazty/Flipper_Apps/main/pword/images/1.png)

![3p](https://raw.githubusercontent.com/dagnazty/Flipper_Apps/main/pword/images/2.png)

![4p](https://raw.githubusercontent.com/dagnazty/Flipper_Apps/main/pword/images/3.png)

## Technical Details

- The app is written in C, tested on OFW and Momentum Firmware.

[Official Firmware](https://github.com/flipperdevices/flipperzero-firmware)

[Momemntum Firmware](https://github.com/Next-Flip/Momentum-Firmware)

## Installation

To install this app on your Flipper Zero:
1. Clone this repository to your local machine. ```git clone https://github.com/dagnazty/Flipper_Apps.git```
2. Navigate to the pword app directory within the cloned repository: ```cd Flipper_Apps/main/pword```
3. Ensure ufbt is installed.
   Windows: ```py -m pip install --upgrade ufbt```
   Mac/Linux: ```python3 -m pip install --upgrade ufbt```
4. Build the application using the Flipper Zero SDK: ```ufbt build```
5. To install the application onto your Flipper Zero, use the ufbt tool with the following command: ```ufbt install```
6. Look for pword in Apps/Tools.

## Contributing

Contributions to pword are welcome. Please feel free to fork the repository, make your changes, and submit a pull request.

## License

This project is licensed under the GNU License. See the LICENSE file for details.

## Acknowledgments

- Developed by [dagnazty](https://github.com/dagnazty)
- Thanks to the Flipper Zero community for their support and contributions.

For any questions or suggestions, please open an issue in the GitHub repository.
