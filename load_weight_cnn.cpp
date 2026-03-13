#include "SPIFFS.h"
#include "cnn.h"
void Init_SPIFFS() {
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS mount failed");
    return;
  }
  else {
    Serial.println("SPIFFS mount success");
  }
}

static bool readToken(File &file, String &token)
{
    token = "";

    char c;

    // skip whitespace
    while (file.available())
    {
        c = file.read();
        if (!isspace(c))
        {
            token += c;
            break;
        }
    }

    // read token
    while (file.available())
    {
        c = file.read();
        if (isspace(c))
            break;

        token += c;
    }

    return token.length() > 0;
}

bool Load_CNN_OneKernel_FromTxt(const char* filename, KERNEL_t& kernel, int kernel_index)
{
    File file = SPIFFS.open(filename, "r");

    if (!file)
    {
        Serial.printf("Cannot open %s\n", filename);
        return false;
    }

    String token;

    // ===== HEADER =====

    readToken(file, token);

    if (token != "CNN")
    {
        Serial.println("Header error");
        file.close();
        return false;
    }

    readToken(file, token);
    if (token != "Kernel")
    {
        Serial.println("Header error");
        file.close();
        return false;
    }

    readToken(file, token);
    if (token != "File")
    {
        Serial.println("Header error");
        file.close();
        return false;
    }

    // ===== kernel_index =====

    readToken(file, token); // kernel_index:

    int file_kernel_index;

    readToken(file, token);
    file_kernel_index = token.toInt();

    if (file_kernel_index != kernel_index)
    {
        Serial.printf("Kernel index mismatch file=%d expected=%d\n",
                      file_kernel_index, kernel_index);
    }

    // ===== n_feature_in =====

    readToken(file, token); // n_feature_in:

    int cin;

    readToken(file, token);
    cin = token.toInt();

    if (cin != kernel.n_feature_in)
    {
        Serial.println("Input channel mismatch");
        file.close();
        return false;
    }

    // ===== kernel_size =====

    readToken(file, token); // kernel_size:

    int k1,k2;

    readToken(file, token);
    k1 = token.toInt();

    readToken(file, token); // x

    readToken(file, token);
    k2 = token.toInt();

    if (k1 != kernel.size || k2 != kernel.size)
    {
        Serial.println("Kernel size mismatch");
        file.close();
        return false;
    }

    // ===== bias =====

    readToken(file, token);

    if (token == "bias:")
    {
        readToken(file, token);
        float bias = token.toFloat();

        if (kernel.bias)
            kernel.bias[kernel_index] = bias;
    }
    else
    {
        Serial.println("Bias missing");
        file.close();
        return false;
    }

    // ===== read kernels =====

    for(int c=0;c<cin;c++)
    {
        readToken(file, token);

        String expected = "[C" + String(c) + "]";

        if(token != expected)
        {
            Serial.printf("Tag mismatch %s vs %s\n",
                          token.c_str(), expected.c_str());
            file.close();
            return false;
        }

        for(int ky=0; ky<kernel.size; ky++)
        {
            for(int kx=0; kx<kernel.size; kx++)
            {
                if(!readToken(file, token))
                {
                    Serial.println("Weight read fail");
                    file.close();
                    return false;
                }

                float v = token.toFloat();

                long idx =
                kernel_index * (kernel.size * kernel.size * kernel.n_feature_in) +
                ky * (kernel.size * kernel.n_feature_in) +
                kx * (kernel.n_feature_in) +
                c;

                kernel.data[idx] = v;
            }
        }
    }

    file.close();
    return true;
}

bool Load_CNN_AllKernels_FromTxtFiles(const char* prefix, KERNEL_t& kernel)
{
    char filename[64];

    for(int f=0; f<kernel.n_kernel; f++)
    {
        sprintf(filename,"%s_k%02d.txt",prefix,f);

        if(!Load_CNN_OneKernel_FromTxt(filename,kernel,f))
        {
            Serial.printf("Fail load kernel %d\n",f);
            return false;
        }
    }

    return true;
}