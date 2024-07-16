# PA2a Container Build (and Run) Instructions

## Step 1. Clone the git repo by using either of the commands below:

***Windows: You need to run this from an Ubuntu shell in WSL, not from Powershell***

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/ucsd-cse123-wi24/project2a-container.git

# Or, clone and update submodules after clone
git clone https://github.com/ucsd-cse123-wi24/project2a-container.git
git submodule update --init --recursive
```

## Step 2: Build and run the Docker container

### Linux and macOS M1/M2

```bash
# Create a volume for your docker container so your project directory will live when your container dies
docker volume create cse123pa2_data
# Build the docker container for the project
docker build -t cse123pa2 --target linux .
# Run the docker container for the project. Run this each time you work on the project.
# You should be able to attach to the running container in VSCode (or using the exec comand below) after this is executed.
docker run -d --rm --privileged -it --name pa2 -v /lib/modules:/lib/modules -v cse123pa2_data:/project-base -t cse123pa2
```
```bash
# To open a shell inside the running container, in each terminal run the following:
docker exec -it pa2 bash
# You can now clone the github repo into your project from inside the Docker container's bash shell
```

### Windows (You need to use both Ubuntu shell and Powershell)

Ubuntu Shell:

#### For Ubuntu Shell user, make sure inside the Docker Desktop -> Settings -> Resource -> WSL Integration, your "ubuntu" button is on.
```bash
# Create a volume for your docker container so your project directory will live when your container dies
docker volume create cse123pa2_data
# Build the docker container for the project (this will take ~30mins to build the new kernel)
docker build -t cse123pa2 --target windows .
# Run the docker container for the project. Run this each time you work on the project.
# You should be able to attach to the running container in VSCode (or using the exec comand below) after this is executed 
docker run -d --rm --privileged -it --name pa2 -v cse123pa2_data:/project-base -t cse123pa2
# Copy the new kernel out of the docker image into WSL
docker cp pa2:/wsl-kernel/vmlinux /tmp/vmlinux
```

Powershell:
```bash
# Switch to your windows home directory
cd ~
# Copy the kernel image out of WSL into your Windows home directory
wsl cp /tmp/vmlinux cse123pa2-kernel
# Launch notepad to edit your wsl config file
notepad .wslconfig
```

In your .wslconfig write the following text save it in notepad (replace YourUserName with your actual windows user name):
```
[wsl2]
kernel=C:\\Users\\YourUserName\\cse123pa2-kernel
```

Powershell:
```
# Shut down wsl (Docker will crash and force a restart of wsl if you have it running)
wsl --shutdown
```

Ubuntu Shell:
```bash
# Run the docker container again
docker run -d --rm --privileged -it --name pa2 -v cse123pa2_data:/project-base -t cse123pa2

# To open a shell inside the running container, in each terminal run the following (or attach to the running container using VSCode):
docker exec -it pa2 bash
# Inside the container you may need to switch to the /project-base folder
root@docker$ cd /project-base

# You can now clone the github repo into your project from inside the Docker container's bash shell and run the rest of the commands!
```

