#include "stdio.h"
#include "city.pb.h"

using namespace mycity;

char buffer[512];
int main()
{
	memset(buffer, 0, 512);
	City city;
	city.set_name("hello my city!");
	mycity::Person* person = city.add_persons();
	if (person)
	{
		person->set_name("zcg");
		person->set_weight(80);
		person->set_year(18);
	}
	person = NULL;
	person = city.add_persons();
	if (person)
	{
		person->set_name("zcg2");
		person->set_weight(75);
		person->set_year(20);
	}

	std::string strData = city.SerializeAsString();
	int size = city.ByteSize();
	city.SerializeToArray(buffer, 512);
	printf("after Serialize size:%d, bytesize:%d \n", strData.length(), size);

	City outCity;
	///outCity.ParseFromArray(buffer, size);
	outCity.ParseFromString(strData);
	printf("ParseFromArray cityname:%s \n", outCity.name().c_str());
	for (int i = 0; i < outCity.persons_size(); i++)
	{
		const Person& temp_person = outCity.persons(i);
		printf("ParseFromArray person%d, name:%s, weight:%d, year:%d \n", i, temp_person.name().c_str(), temp_person.weight(), temp_person.year());
	}

	char a = 0;
	putchar(a);

	return 0;
}