package tw.edu.cgu.ai.zenbo;

import java.io.IOException;
import java.net.Socket;

public class ReconnectingClient {

    private String host;
    private int port;
    private Socket socket;
    private boolean running = true;
    private long initialDelay = 1000; // 1 second
    private long maxDelay = 30000;   // 30 seconds
    private int maxRetries = 5;      // Maximum reconnection attempts

    public ReconnectingClient(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public void start() {
        int retryCount = 0;
        long currentDelay = initialDelay;

        while (running) {
            try {
                System.out.println("Attempting to connect to " + host + ":" + port);
                socket = new Socket(host, port);
                System.out.println("Connected successfully!");
                retryCount = 0; // Reset retry count on successful connection
                currentDelay = initialDelay;

                // Handle communication with the server in a separate thread
                new Thread(this::handleConnection).start();
                return; // Exit the reconnection loop after successful connection
            } catch (IOException e) {
                System.err.println("Connection failed: " + e.getMessage() + ". Retrying in " + currentDelay / 1000 + " seconds...");
                retryCount++;

                if (maxRetries > 0 && retryCount >= maxRetries) {
                    System.err.println("Maximum reconnection attempts reached. Giving up.");
                    running = false;
                    break;
                }

                try {
                    Thread.sleep(currentDelay);
                } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    running = false;
                    break;
                }

                // Implement exponential backoff
                currentDelay = Math.min(currentDelay * 2, maxDelay);
            }
        }
    }

    private void handleConnection() {
        try {
            // Implement your communication logic here (reading from and writing to the socket)
            // This loop should run as long as the connection is alive

            // Example: Read from the server
            java.io.BufferedReader reader = new java.io.BufferedReader(new java.io.InputStreamReader(socket.getInputStream()));
            String message;
            while ((message = reader.readLine()) != null) {
                System.out.println("Received: " + message);
            }
            System.out.println("Server disconnected.");
            reconnect(); // Initiate reconnection when the server disconnects
        } catch (IOException e) {
            System.err.println("Error in connection handling: " + e.getMessage());
            reconnect(); // Initiate reconnection on communication error
        } finally {
            closeSocket();
        }
    }

    private void reconnect() {
        // You might want to add a delay here before immediately trying to reconnect
        try {
            Thread.sleep(initialDelay);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        start(); // Call the start method again to initiate reconnection
    }

    private void closeSocket() {
        if (socket != null && !socket.isClosed()) {
            try {
                socket.close();
                System.out.println("Socket closed.");
            } catch (IOException e) {
                System.err.println("Error closing socket: " + e.getMessage());
            } finally {
                socket = null;
            }
        }
    }

    public void stop() {
        running = false;
        closeSocket();
    }
}
