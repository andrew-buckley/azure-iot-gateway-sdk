import com.google.gson.Gson;
import com.microsoft.azure.gateway.core.GatewayModule;
import com.microsoft.azure.gateway.core.MessageBus;
import com.microsoft.azure.gateway.messaging.Message;
import com.microsoft.azure.iothub.*;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 * Created by andbuc on 6/13/2016.
 */
public class IotHubAmqpModule extends GatewayModule{

    private Map<String, DeviceClient> clients = null;
    private IotHubAmqpModuleConfiguration config = null;
    private Gson gson = null;

    /**
     * Constructs a {@link GatewayModule} from the provided address and {@link MessageBus}. A {@link GatewayModule} should always call this super
     * constructor before any module-specific constructor code.
     *
     * @param address       The address of the native module pointer
     * @param bus           The {@link MessageBus} to which this module belongs
     * @param configuration The module-specific configuration
     */
    public IotHubAmqpModule(long address, MessageBus bus, String configuration) {
        super(address, bus, configuration);

        this.clients = new HashMap<>();
        this.gson = new Gson();
        this.config = gson.fromJson(configuration, IotHubAmqpModuleConfiguration.class);
    }

    public boolean sendMessage(Message message, String deviceID, String deviceKey){
        //Attempt to retrieve a client for this device
        DeviceClient client = this.clients.get(deviceID);

        //If no device client yet exists for this deviceID, create one.
        if(client == null){
            try {
                String connectionString = this.constructConnectionString(this.config.getIotHubName(), this.config.getIotHubSuffix(), deviceID, deviceKey);
                client = new DeviceClient(connectionString, IotHubClientProtocol.AMQPS);
                MessageCallback callback = new MessageCallback();
                client.setMessageCallback(callback, this);
                client.open();
                clients.put(deviceID, client);
            } catch (URISyntaxException e) {
                System.out.println("URISyntaxException");
                return false;
            } catch (IOException e) {
                System.out.println("IOException");
                return false;
            }
        }

        //Asynchronously send message
        EventCallback eventCallback = new EventCallback();
        client.sendEventAsync(this.gatewayMessageToIotHubMessage(message), eventCallback, null);
        return true;
    }

    public String constructConnectionString(String hostName, String suffix, String deviceID, String deviceKey){
        StringBuilder builder = new StringBuilder();
        builder.append("HostName=")
            .append(hostName)
            .append(".")
            .append(suffix)
            .append(";")
            .append("DeviceId=")
            .append(deviceID)
            .append(";")
            .append("SharedAccessKey=")
            .append(deviceKey);

        return builder.toString();
    }

    public com.microsoft.azure.iothub.Message gatewayMessageToIotHubMessage(Message message){
        com.microsoft.azure.iothub.Message iotHubMessage = new com.microsoft.azure.iothub.Message(message.getContent());

        Map<String, String> properties = message.getProperties();

        Set<String> keySet = properties.keySet();
        for(String key : keySet){
            if(!key.equals("macAddress") && !key.equals("deviceKey")) {
                iotHubMessage.setProperty(key, properties.get(key));
            }
        }

        return iotHubMessage;
    }

    @Override
    public void receive(Message message) {
        Map<String, String> properties = message.getProperties();
        if(properties.containsKey("source") &&
                properties.get("source").equals("mapping") &&
                properties.containsKey("deviceName") &&
                properties.containsKey("deviceKey")){
            if(!this.sendMessage(message, properties.get("deviceName"), properties.get("deviceKey"))){
                System.out.println("IotHubAmqpModule could not send message: " + message.toString());
            }
        }
    }

    @Override
    public void destroy() {
        for(DeviceClient client : this.clients.values()){
            try {
                client.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private static class EventCallback implements IotHubEventCallback{

        @Override
        public void execute(IotHubStatusCode responseStatus, Object callbackContext) {
            //Do nothing for now
        }
    }

    private static class MessageCallback implements com.microsoft.azure.iothub.MessageCallback{

        @Override
        public IotHubMessageResult execute(com.microsoft.azure.iothub.Message message, Object callbackContext) {
            //Cast the callback context as an IotHubAmqpModule object
            IotHubAmqpModule parentModule = (IotHubAmqpModule)callbackContext;

            //Set the default result
            IotHubMessageResult result = IotHubMessageResult.COMPLETE;

            //Get & Set properties
            HashMap<String, String> callbackMessageProperties = new HashMap<>();
            MessageProperty[] properties = message.getProperties();
            callbackMessageProperties.put("source", "IoTHubHTTP");

            //Adds all properties
            for(MessageProperty p : properties){
                callbackMessageProperties.put(p.getName(), p.getValue());
            }

            Message callbackMessage = new Message(message.getBytes(), callbackMessageProperties);

            try {
                parentModule.publishMessage(callbackMessage);
            } catch (IOException e) {
                System.out.println("IOException while sending callback message. The message was not sent.");
                result = IotHubMessageResult.REJECT;
            }
            return result;
        }
    }

    private class IotHubAmqpModuleConfiguration{
        String IoTHubName = null;
        String IoTHubSuffix = null;
        public IotHubAmqpModuleConfiguration(){}

        public String getIotHubName(){
            return IoTHubName;
        }

        public String getIotHubSuffix(){
            return IoTHubSuffix;
        }
    }
}
