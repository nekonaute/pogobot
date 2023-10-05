# Pogobot Wall App
This standalone app allows to connect up to 4 remotes in parallel.
This application is experimental.

The app allows you to identify each ttyUSB by a different color.
Then identify on the layout where goes each ttyUSB and click on "update device name".
Now you have each device interface identified with the name of the Wall.

Autocompletion is available for the most used command.

## installation

sudo apt-get install nodejs
sudo apt-get install npm
npm install electron

// change to node v18
npm install nvm 
wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.5/install.sh | bash
nvm install 18

npm install serialport
npm install --save-dev @electron-forge/cli

## local launch

cd pogoWallApp
npm start

## build the app (.deb)

cd pogoWallApp
npm run make

