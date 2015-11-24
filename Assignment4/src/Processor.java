import java.util.Random;
public class Processor {
		public Cache cache;
		int selectedBlock;
		boolean read;
		int coinToss;
		int accessCounter;
		int stateChanges;
		Random rand = new Random();
		Processor(int cacheSize,int blockSize)
		{	
			rand.setSeed(2);
			cache = new Cache(cacheSize,blockSize);
			accessCounter = 0;
			stateChanges = 0;
		}
		// function for generating the random instruction
		void generateInstruction()
		{
			selectedBlock = rand.nextInt(1024);
			coinToss = rand.nextInt(2);
			if(coinToss==1)
			{
				read = true;
			}
			else read = false;
		}	
}
