"""One-client, loopback-only Streamer.bot transport fixture (standard library).

Run alongside verify_streamer_bot_connection.py. This never connects to Twitch.
"""
import base64
import hashlib
import json
import socket
import struct
import time
from datetime import datetime, timedelta, timezone

HOST, PORT = "127.0.0.1", 18880


def exact(sock, count):
    result = b""
    while len(result) < count:
        part = sock.recv(count - len(result))
        if not part:
            raise EOFError
        result += part
    return result


def send(sock, obj):
    payload = json.dumps(obj).encode()
    head = bytes([0x81, len(payload)]) if len(payload) < 126 else bytes([0x81, 126]) + struct.pack("!H", len(payload))
    sock.sendall(head + payload)


def receive(sock):
    first, second = exact(sock, 2)
    size = second & 127
    if size == 126:
        size = struct.unpack("!H", exact(sock, 2))[0]
    elif size == 127:
        size = struct.unpack("!Q", exact(sock, 8))[0]
    assert size <= 8192
    mask = exact(sock, 4) if second & 128 else None
    data = exact(sock, size)
    if mask:
        data = bytes(value ^ mask[index % 4] for index, value in enumerate(data))
    if first & 15 == 8:
        raise EOFError
    assert first & 15 == 1
    return json.loads(data)


def payload(identity, value):
    return dict(schema_version=1, topic="rtcqt.visuals", command="test", value=value,
                request_id=identity, issued_at=datetime.now(timezone.utc).isoformat())


with socket.socket() as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(1)
    server.settimeout(60)
    print(f"Mock Streamer.bot listening on {HOST}:{PORT}", flush=True)
    conn, _ = server.accept()
    with conn:
        conn.settimeout(20)
        header = b""
        while not header.endswith(b"\r\n\r\n"):
            header += exact(conn, 1)
            assert len(header) <= 8192
        headers = dict(line.split(":", 1) for line in header.decode().split("\r\n")[1:] if ":" in line)
        key = next(value.strip() for name, value in headers.items() if name.lower() == "sec-websocket-key")
        accept = base64.b64encode(hashlib.sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()).decode()
        conn.sendall(("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
                      "Connection: Upgrade\r\nSec-WebSocket-Accept: " + accept + "\r\n\r\n").encode())
        send(conn, {"request": "Hello", "info": {"version": "fixture"}})
        subscribe = receive(conn)
        assert subscribe["request"] == "Subscribe" and subscribe["events"] == {"General": ["Custom"]}, subscribe
        send(conn, {"id": subscribe["id"], "status": "ok"})
        direct = payload("direct", "direct socket test")
        send(conn, direct)
        send(conn, direct)
        stale = payload("stale", "must be ignored")
        stale["issued_at"] = (datetime.now(timezone.utc) - timedelta(seconds=30)).isoformat()
        send(conn, stale)
        send(conn, {"event": {"source": "General", "type": "Custom"},
                    "data": {"data": json.dumps(payload("wrapped", "wrapped socket test"))}})
        try:
            while True:
                receive(conn)
        except (EOFError, ConnectionResetError):
            print("Mock transport exchange PASSED", flush=True)
