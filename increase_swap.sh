#!/bin/bash
set -e

swapoff /swapfile || true
rm -f /swapfile
fallocate -l 2G /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile