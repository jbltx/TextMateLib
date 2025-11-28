#!/bin/bash
# Serve the playground with a local HTTP server

PORT=${1:-8000}

echo "Starting HTTP server on port $PORT..."
echo "Open http://localhost:$PORT in your browser"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Try different server options
if command -v python3 &> /dev/null; then
    python3 -m http.server $PORT
elif command -v python &> /dev/null; then
    python -m SimpleHTTPServer $PORT
elif command -v npx &> /dev/null; then
    npx http-server -p $PORT
else
    echo "Error: No HTTP server available!"
    echo "Please install Python or Node.js"
    exit 1
fi
