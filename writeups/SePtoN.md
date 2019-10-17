# gallery

It is a web-service, emulating small private gallery. Users can publish paintings samples and set a reward for other users, who bring them paintings that look similar to the original one.

Gallery service uses specially trained neural network which produces embeddings for the input patinigs. Embeddings are vectors in N-dimensional space and are used represent the paintings. Distance between embedding vectors is a measure of similarity between the input paintings.


## Embedding recovery

To get a flag ("reward" in terms of this service) hacker needs to send an image, which embedding would be close enough to the embedding of the original image. But hacker does not know neither original image, nor it's embedding, so first he needs to recover the embedding.
Hacker can do that, because he get's the distance between the target embedding and the embedding of arbitrary image he sends to the service. Also hacker knows the embedding of his arbitrary image, because he has the same neural network.
If hacker get's enough pairs of (embedding, distance), he has a system of equations (where each equation is distance equation in N-dimensional space).
Cause space has 16 dimensios, hacker needs only 17 such pairs to solve it: just subtract one of these equations from the others to get linear equation instead of square. And then hacker easily solves a set of 16 equations with 16 variables and gets those variables.
These variables are actually the embedding.

## Backprop

Hacker now needs to get image, which embedding will be close enough to the calculated embedding.
To do that hacker can send to the neural network some image, calculate the difference between the result and target embedding, and backpropogate this diff back through the network to adjust the input image in the way it decreases the difference. Doing it repetatively hacker finally obtains the input image with embedding close enough to the target.

See full exploit [here](https://github.com/HackerDom/proctf-2019/tree/master/checkers/gallery/gallery.exploit.ipynb).

## Patching

To protect from this vuln, embeddings from the service should differ from the embedding, hacker can obtain from his own network.
This can be done e.g. by training the network in a way, that embeddings from similar images will still be close, but the exact coordinates of the embeddings differ from those, hacker get from his network.