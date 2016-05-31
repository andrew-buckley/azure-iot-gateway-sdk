/*
 * Copyright (c) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 */
package com.microsoft.azure.gateway.messaging;

import java.io.*;
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
        return null;
    }

    public String getContent(){
        return null;
    }

    public byte[] toByteArray(){
        return null;
    }

    private void fromByteArray(byte[] serializedMessage) throws IOException {
        ByteArrayInputStream bis = new ByteArrayInputStream(serializedMessage);
        DataInputStream dis = new DataInputStream(bis);
        //BufferedReader br = new BufferedReader(new InputStreamReader(dis));

        //Get Header
        byte header1 = dis.readByte();
        byte header2 = dis.readByte();
        if(header1 == 0xA1 && header2 == 0x60){
            int arraySize = dis.readInt();
            int propCount = dis.readInt();
            for(int count = 0; count < propCount; count++){

            }
        }

        this.content = serializedMessage;
    }
}
