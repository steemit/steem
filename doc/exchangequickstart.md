Exchange Quickstart
-------------------

System Requirements: A minimum of 16GB of RAM and at least 50GB of fast local SSD storage. STEEM is currently the most active blockchain in the world and handles an incredibly large amount of transactions per second, as such, it requires fast storage to run efficiently.

We recommend using docker to both build and run steem for exchanges. Docker is the world's leading containerization platform and using it guarantees that your build and run environment is identical to what our developers use. You can still build from source and you can keep both blockchain data and wallet data outside of the docker container. The instructions below will show you how to do this in just a few easy steps.

### Install docker and git (if not already installed)

On Ubuntu 16.04+:
```
apt-get update && apt-get install git docker.io -y
```

On other distributions you can install docker with the native package manager or with the script from get.docker.com:
```
curl -fsSL get.docker.com -o get-docker.sh
sh get-docker.sh
```

### Clone the steem repo

Pull in the steem repo from the official source on github and then change into the directory that's created for it.
```
git clone https://github.com/steemit/steem
cd steem
```

### Build the image from source with docker

Docker isn't just for downloading already built images, it can be used to build from source the same way you would otherwise build. By doing this you ensure that your build environment is identical to what we use to develop the software. Use the below command to start the build:

```
docker build -t=steemit/steem .
```

Don't forget the `.` at the end of the line which indicates the build target is in the current directory.

When the build completes you will see a message indicating that it is 'successfully built'.

### Create directories to store blockchain and wallet data outside of docker

For re-usability, you can create directories to store blockchain and wallet data and easily link them inside your docker container.

```
mkdir blockchain
mkdir steemwallet
```

### Run the container

The below command will start a daemonized instance opening ports for p2p and RPC  while linking the directories we created for blockchain and wallet data inside the container. Fill in `TRACK_ACCOUNT` with the name of your exchange account that you want to follow. The `-v` flags are how you map directories outside of the container to the inside, you list the path to the directories you created earlier before the `:` for each `-v` flag. The restart policy ensures that the container will automatically restart even if your system is restarted.

```
docker run -d --name steemd-exchange --env TRACK_ACCOUNT=nameofaccount -p 2001:2001 -p 8090:8090 -v /path/to/steemwallet:/var/steemwallet -v /path/to/blockchain:/var/lib/steemd/blockchain --restart always steemit/steem
```

You can see that the container is running with the `docker ps` command.

To follow along with the logs, use `docker logs -f`.

Initial syncing will take between 6 and 48 hours depending on your equipment, faster storage devices will take less time and be more efficient. Subsequent restarts will not take as long.

### Running the cli_wallet

The command below will run the cli_wallet from inside the running container while mapping the `wallet.json` to the directory you created for it on the host.

```
docker exec -it steemd-exchange /usr/local/steemd-default/bin/cli_wallet -w /var/steemwallet/wallet.json
```