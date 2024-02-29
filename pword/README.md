# pword
This application for the Flipper Zero generates random strings of 8 or 16 characters and provides a simple menu to navigate through.

## How It Works

Upon launching the app, the user is presented with a menu to select the desired action:
1. Generate an 8-character random string
2. Generate a 16-character random string
3. View the About page

The application utilizes the device's hardware buttons for navigation:
- **Up/Down** to navigate through the menu
- **OK** to select an option or generate a new string in the string generation view
- **Left/Right** in the string generation view to generate a new random string
- **Back** to return to the previous menu or exit the app

## Technical Details

- The app is written in C, tested on OFW and Momentum Firmware.
- Random strings are generated from a predefined charset, ensuring a mix of upper and lower case letters, numbers, and special characters.
- The application state is managed through an enum, allowing for clear transitions between menu navigation, string generation, and viewing the About page.
- Utilizes dynamic memory allocation for the application context, ensuring clean setup and teardown of the app's resources.

## Installation

To install this app on your Flipper Zero:
1. Clone this repository to your local machine. ```git clone https://github.com/dagnazty/Flipper_Apps.git```
2. Navigate to the pword app directory within the cloned repository: ```cd Flipper_Apps/main/pword```
3. Ensure ufbt is installed
   Windows: ```py -m pip install --upgrade ufbt```
   Mac/Linux: ```python3 -m pip install --upgrade ufbt```
4. Build the application using the Flipper Zero SDK: ```ufbt build```
5. To install the application onto your Flipper Zero, use the ufbt tool with the following command: ```ufbt install```
6. Look for app in Apps/Examples.

## Contributing

Contributions to pword are welcome. Please feel free to fork the repository, make your changes, and submit a pull request.

## License

This project is licensed under the GNU License. See the LICENSE file for details.

## Acknowledgments

- Developed by dag
- Thanks to the Flipper Zero community for their support and contributions.

For any questions or suggestions, please open an issue in the GitHub repository.
