/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.messaging;

import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public final class Message {

    private Map<String, String> properties;

    private byte[] content;

    public Message(String content, Map<String, String> properties){
        this.content = content.getBytes();
        this.properties = properties;
    }

    public Message(String content){
        this.content = content.getBytes();
        this.properties = null;
    }

    public Message(byte[] serializedMessage){
        try {
            fromByteArray(serializedMessage);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public Map<String, String> getProperties(){
        return properties;
    }

    public byte[] getContent(){
        return content;
    }

    public byte[] toByteArray() throws IOException {
        ByteArrayOutputStream _bos = new ByteArrayOutputStream();
        DataOutputStream _dos = new DataOutputStream(_bos);

        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        DataOutputStream dos = new DataOutputStream(bos);

        //Write Properties Count
        _dos.writeInt(this.properties.size());

        //Write Properties
        Object[] keys = this.properties.keySet().toArray();
        for(int index = 0; index < keys.length; index++){
            _dos.writeBytes((String)keys[index]);
            _dos.writeByte('\0');
            _dos.writeBytes(this.properties.get(keys[index]));
            _dos.writeByte('\0');
        }

        //Write message content size
        _dos.writeInt(this.content.length);

        //Write message content
        _dos.write(this.content);

        byte[] _result = _bos.toByteArray();

        //Write Header
        dos.writeByte(0xA1);
        dos.writeByte(0x60);

        //Write ArraySize
        dos.writeInt(_result.length + 6);
        dos.write(_result);
        byte[] result = bos.toByteArray();

        return result;
    }

    public String toString(){
        return "Content: " + new String(this.content) + "\nProperties: " + this.properties.toString();
    }

    private void fromByteArray(byte[] serializedMessage) throws IOException {
        ByteArrayInputStream bis = new ByteArrayInputStream(serializedMessage);
        DataInputStream dis = new DataInputStream(bis);

        //Get Header
        byte header1 = dis.readByte();
        byte header2 = dis.readByte();
        if(header1 == (byte)0xA1 && header2 == (byte)0x60){
            int arraySize = dis.readInt();
            //if(arraySize < 14) {
                int propCount = dis.readInt();

                if (propCount > 0) {
                    this.properties = new HashMap<String, String>();
                    for (int count = 0; count < propCount; count++) {
                        byte[] key = readNullTerminatedByte(bis);
                        byte[] value = readNullTerminatedByte(bis);
                        this.properties.put(new String(key), new String(value));
                    }
                }

                int contentLength = dis.readInt();
                byte[] content = new byte[contentLength];
                dis.readFully(content);
                this.content = content;
            //}
            //else{
            //    throw new IllegalArgumentException("Invalid byte array size.");
            //}
        }
    }

    /**
     * Returns the first null-terminated ('\0') sub-array.
     * @param bis
     * @return
     */
    private byte[] readNullTerminatedByte(ByteArrayInputStream bis){
        ArrayList<Byte> byteArray = new ArrayList<Byte>();

        byte b = (byte) bis.read();

        while(b != '\0' && b != -1){
            byteArray.add(b);
            b = (byte)bis.read();
        }

        byte[] result = new byte[byteArray.size()];
        for(int index = 0; index < result.length; index++){
            result[index] = byteArray.get(index);
        }

        return result;
    }
}
