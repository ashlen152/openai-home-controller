# Start from a slim Python base image
FROM python:3.11-slim

# Set environment variable to avoid Python buffering issues
ENV PYTHONUNBUFFERED=1

# Install essential tools and dependencies for PlatformIO & Arduino
RUN apt-get update && apt-get install -y \
    git \
    gcc \
    g++ \
    make \
    libusb-1.0-0 \
    udev \
    avrdude \
    curl \
    unzip \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO using pip
RUN pip install -U platformio

# Set working directory
WORKDIR /project

# Copy your PlatformIO project into the image
COPY . /project

# Volume mount USB for uploads (Arduino boards)
VOLUME ["/dev"]

# Default build command (can be overridden)
CMD ["platformio", "run"]
