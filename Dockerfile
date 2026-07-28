FROM python:3.10-slim

# Install cmake and g++ for building the C++ engine
RUN apt-get update && apt-get install -y cmake g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy all project files
COPY . .

# Build the C++ engine and generate opening book
# Build the C++ engine and generate opening book
RUN mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release
RUN cd build && ./make_book ../data/openings.txt blunderbot_book.bin && cp ../Blunderbot.nnue .

# Install Python requirements
RUN pip install --no-cache-dir -r requirements.txt

# Expose port (default 8000)
EXPOSE 8000

# Start server using the port provided by the environment (needed for Render/Fly.io)
CMD sh -c "uvicorn server.server:app --host 0.0.0.0 --port ${PORT:-8000}"
